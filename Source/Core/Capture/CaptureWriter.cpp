#include "CaptureWriter.h"

#include <algorithm>
#include <limits>

namespace conduit
{

namespace
{
    /** Datei blockweise auf die Zielgröße erweitern — ENOSPC schlägt hier
        fehl, nicht erst mitten in den Audiodaten; zusammenhängende
        Allokation reduziert Fragmentierung. */
    bool preallocateInBlocks (juce::FileOutputStream& stream, std::int64_t targetBytes,
                              juce::Thread& thread)
    {
        for (std::int64_t pos = 0; pos < targetBytes;)
        {
            if (thread.threadShouldExit())
                return false;

            pos = juce::jmin (targetBytes, pos + CaptureWriter::preallocBlockBytes);
            if (! stream.setPosition (pos - 1) || ! stream.writeByte (0))
                return false;
        }

        stream.flush();
        return true;
    }
}

//==============================================================================
CaptureWriter::CaptureWriter()
    : juce::Thread ("Capture Writer")
{
    startThread();
}

CaptureWriter::~CaptureWriter()
{
    stopThread (10'000);

    // Nie verarbeitete Jobs: Pins trotzdem lösen (Halte-Protokoll), sonst
    // blieben Puffersätze für immer gepinnt
    for (auto& job : queue)
        if (job->releaseResources)
            job->releaseResources();
    queue.clear();
}

//==============================================================================
void CaptureWriter::enqueueJob (Job job)
{
    // Vor dem Einreihen inkrementieren: isBusy() ist konservativ true,
    // sobald ein Job unterwegs ist
    pendingJobs.fetch_add (1, std::memory_order_release);
    {
        const juce::ScopedLock lock (queueLock);
        queue.push_back (std::make_unique<Job> (std::move (job)));
    }
    notify();
}

std::unique_ptr<CaptureWriter::Job> CaptureWriter::popNextJob()
{
    const juce::ScopedLock lock (queueLock);
    if (queue.empty())
        return nullptr;

    auto job = std::move (queue.front());
    queue.erase (queue.begin());
    return job;
}

//==============================================================================
void CaptureWriter::run()
{
    while (! threadShouldExit())
    {
        auto job = popNextJob();
        if (job == nullptr)
        {
            wait (-1);  // stopThread()/enqueueJob() rufen notify()
            continue;
        }

        const auto report = processJob (*job);

        if (job->releaseResources)
            job->releaseResources();
        pendingJobs.fetch_sub (1, std::memory_order_release);

        if (onJobFinished)
            onJobFinished (report);
    }
}

//==============================================================================
std::uint64_t CaptureWriter::computeExportStart (const std::vector<Task>& tasks) noexcept
{
    auto exportStart = std::numeric_limits<std::uint64_t>::max();
    for (const auto& task : tasks)
        exportStart = juce::jmin (exportStart, task.startPosition);

    return tasks.empty() ? 0 : exportStart;
}

std::uint64_t CaptureWriter::computePadSamples (std::uint64_t taskStart,
                                                std::uint64_t exportStart) noexcept
{
    return taskStart >= exportStart ? taskStart - exportStart : 0;
}

juce::String CaptureWriter::makeFileName (const juce::String& timestamp,
                                          const juce::String& trackName, int takeNumber)
{
    return timestamp + "_" + trackName + "_" + juce::String (takeNumber) + ".wav";
}

//==============================================================================
CaptureWriter::Report CaptureWriter::processJob (const Job& job)
{
    Report report;
    report.directory   = job.directory;
    report.exportStart = computeExportStart (job.tasks);

    struct ActiveTask
    {
        const Task* task = nullptr;
        juce::File file;
        juce::FileOutputStream* stream = nullptr;  // gehört dem Writer (unten)
        std::unique_ptr<juce::AudioFormatWriter> writer;
        std::uint64_t padFrames = 0;
        std::uint64_t totalFrames = 0;
        std::uint64_t framesWritten = 0;
        std::uint32_t lastHeaderFlush = 0;
        bool finished = false;
        juce::String error;
    };

    const auto failTask = [] (ActiveTask& t, const juce::String& why)
    {
        t.error = why;
        t.finished = true;
        t.writer.reset();      // schließt den Stream
        t.stream = nullptr;
        t.file.deleteFile();   // abgebrochene Kanäle hinterlassen keine Datei
    };

    const auto finalizeTask = [] (ActiveTask& t)
    {
        t.writer->flush();                              // Header aktualisieren
        const auto finalSize = t.stream->getPosition(); // exakte Endgröße
        t.writer.reset();                               // schreibt Header, schließt
        t.stream = nullptr;

        // Preallokations-Überhang exakt abschneiden
        juce::FileOutputStream truncStream (t.file);
        if (! truncStream.failedToOpen() && truncStream.setPosition (finalSize))
            truncStream.truncate();

        t.finished = true;  // error leer == Erfolg
    };

    std::vector<ActiveTask> active (job.tasks.size());
    juce::WavAudioFormat wavFormat;
    const auto bytesPerSample = juce::jmax (1, job.bitDepth / 8);
    const auto creationTime   = juce::Time::getCurrentTime();
    const auto timestamp      = creationTime.formatted ("%Y-%m-%d_%H%M%S");

    job.directory.createDirectory();

    // -- Phase 1: Dateien anlegen (Preallokation in Blöcken), BWF-Writer ------
    for (size_t i = 0; i < job.tasks.size(); ++i)
    {
        auto& t = active[i];
        t.task        = &job.tasks[i];
        t.padFrames   = computePadSamples (t.task->startPosition, report.exportStart);
        t.totalFrames = t.padFrames + (t.task->endPosition - t.task->startPosition);

        auto file = job.directory.getChildFile (makeFileName (timestamp, t.task->trackName,
                                                              job.takeNumber));
        if (file.existsAsFile())
            file = file.getNonexistentSibling();
        t.file = file;

        auto stream = std::make_unique<juce::FileOutputStream> (file);
        if (stream->failedToOpen())
        {
            failTask (t, "Datei konnte nicht angelegt werden: " + file.getFullPathName());
            continue;
        }

        const auto estimatedBytes = 4096  // Header-Slack (RIFF/fmt/bext/data)
                                  + static_cast<std::int64_t> (t.totalFrames)
                                  * static_cast<std::int64_t> (bytesPerSample);
        if (! preallocateInBlocks (*stream, estimatedBytes, *this)
            || ! stream->setPosition (0))
        {
            stream.reset();
            failTask (t, "Preallokation fehlgeschlagen (Speicherplatz?): " + file.getFileName());
            continue;
        }

        // TimeReference = exportStart für ALLE Dateien — das ist das Alignment
        const auto metadata = juce::WavAudioFormat::createBWAVMetadata (
            "Conduit Capture " + t.task->trackName, "Conduit", {}, creationTime,
            static_cast<juce::int64> (report.exportStart), {});

        t.stream = stream.get();
        t.writer.reset (wavFormat.createWriterFor (stream.get(), job.sampleRate, 1,
                                                   job.bitDepth, metadata, 0));
        if (t.writer == nullptr)
        {
            t.stream = nullptr;
            stream.reset();
            failTask (t, "WAV-Writer konnte nicht erstellt werden (Bit-Tiefe "
                          + juce::String (job.bitDepth) + ")");
            continue;
        }

        stream.release();  // der AudioFormatWriter besitzt den Stream jetzt
        t.lastHeaderFlush = juce::Time::getMillisecondCounter();

        if (t.totalFrames == 0)
            finalizeTask (t);  // leerer (aber gültiger) Bereich → leere Datei
    }

    // -- Phase 2: Chunks schreiben, vollster Ring zuerst (Überholschutz) ------
    juce::AudioBuffer<float> chunkBuffer (1, chunkSamples);

    while (! threadShouldExit())
    {
        ActiveTask* next = nullptr;
        double worstFill = -1.0;

        for (auto& t : active)
        {
            if (t.finished)
                continue;

            double fill = 0.0;
            const auto& source = t.task->source;
            if (source.ringCapacitySamples > 0 && source.getCurrentEnd)
            {
                const auto readPos = t.task->startPosition
                                   + (t.framesWritten > t.padFrames
                                          ? t.framesWritten - t.padFrames : 0);
                const auto liveEnd = source.getCurrentEnd();
                const auto pendingInRing = liveEnd > readPos ? liveEnd - readPos : 0;
                fill = static_cast<double> (pendingInRing)
                     / static_cast<double> (source.ringCapacitySamples);
            }

            if (fill > worstFill)
            {
                worstFill = fill;
                next = &t;
            }
        }

        if (next == nullptr)
            break;  // alle Kanäle fertig (oder abgebrochen)

        auto& t = *next;
        const auto remaining = t.totalFrames - t.framesWritten;
        const auto todo = static_cast<int> (juce::jmin (
            remaining, static_cast<std::uint64_t> (chunkSamples)));

        if (t.framesWritten < t.padFrames)
        {
            // Alignment-Padding: Null-Samples bis zum Kanal-Start
            const auto padTodo = static_cast<int> (juce::jmin (
                t.padFrames - t.framesWritten, static_cast<std::uint64_t> (chunkSamples)));
            chunkBuffer.clear();

            if (! t.writer->writeFromAudioSampleBuffer (chunkBuffer, 0, padTodo))
            {
                failTask (t, "Schreibfehler (Padding): " + t.file.getFileName());
                continue;
            }
            t.framesWritten += static_cast<std::uint64_t> (padTodo);
        }
        else
        {
            const auto readPos = t.task->startPosition + (t.framesWritten - t.padFrames);
            const auto& source = t.task->source;

            // Überholschutz: Abbruch BEVOR der Live-Wrap den Lesebereich
            // erreicht (Marge = Kapazität/8) — siehe Klassendoku
            if (source.ringCapacitySamples > 0 && source.getCurrentEnd)
            {
                const auto capacity = static_cast<std::uint64_t> (source.ringCapacitySamples);
                const auto margin   = capacity / static_cast<std::uint64_t> (overrunMarginDivisor);
                const auto liveEnd  = source.getCurrentEnd();
                const auto pendingInRing = liveEnd > readPos ? liveEnd - readPos : 0;

                if (pendingInRing + margin > capacity)
                {
                    failTask (t, juce::String::fromUTF8 ("Overrun: Aufnahme hat den Export"
                                                         " \xc3\xbc" "berholt (")
                                  + t.file.getFileName() + ")");
                    continue;
                }
            }

            if (! source.read (readPos, chunkBuffer.getWritePointer (0), todo))
            {
                failTask (t, "Quelle nicht mehr lesbar (Overrun oder freigegeben): "
                              + t.file.getFileName());
                continue;
            }

            if (! t.writer->writeFromAudioSampleBuffer (chunkBuffer, 0, todo))
            {
                failTask (t, "Schreibfehler: " + t.file.getFileName());
                continue;
            }
            t.framesWritten += static_cast<std::uint64_t> (todo);
        }

        // Header-Flush alle 10 s — ein Crash kostet höchstens 10 s Material
        const auto nowMs = juce::Time::getMillisecondCounter();
        if (nowMs - t.lastHeaderFlush >= static_cast<std::uint32_t> (headerFlushIntervalMs))
        {
            t.writer->flush();
            t.lastHeaderFlush = nowMs;
        }

        if (t.framesWritten >= t.totalFrames)
            finalizeTask (t);
    }

    // -- Abschluss: Teardown-Abbruch sauber abräumen, Report bauen ------------
    for (auto& t : active)
    {
        if (! t.finished)
            failTask (t, "Export abgebrochen (Teardown)");

        TaskResult result;
        result.trackName    = t.task->trackName;
        result.file         = t.file;
        result.channelIndex = t.task->channelIndex;
        result.success      = t.error.isEmpty();
        result.error        = t.error;

        if (result.success)
            ++report.numSucceeded;
        else
            ++report.numFailed;

        report.tasks.push_back (std::move (result));
    }

    return report;
}

} // namespace conduit
