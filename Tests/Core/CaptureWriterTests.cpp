#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

#include "Core/Capture/CaptureService.h"
#include "Core/Capture/CaptureWriter.h"

using conduit::BufferPool;
using conduit::CaptureChannel;
using conduit::CaptureWriter;
using conduit::PreRollBuffer;

namespace
{

/** Eindeutiger Sample-Wert pro absoluter Position — klein genug, dass die
    24-Bit-Quantisierung des WAV-Roundtrips (Fehler ≤ 2^-24) deutlich unter
    dem Abstand benachbarter Rampenwerte (~7,6e-6) bleibt. */
float rampValue (std::uint64_t position) noexcept
{
    return static_cast<float> (position % 100'003ull) / 131072.0f;
}

constexpr float roundtripTolerance = 1.0e-6f;

void fillRamp (float* dest, int numSamples, std::uint64_t startPosition)
{
    for (int i = 0; i < numSamples; ++i)
        dest[i] = rampValue (startPosition + static_cast<std::uint64_t> (i));
}

/** Eingefrorene Quelle über [start, end) — liefert die Rampe, außerhalb false. */
CaptureWriter::TrackSource makeFrozenRampSource (std::uint64_t start, std::uint64_t end)
{
    CaptureWriter::TrackSource source;
    source.read = [start, end] (std::uint64_t position, float* dest, int numSamples)
    {
        if (position < start || position + static_cast<std::uint64_t> (numSamples) > end)
            return false;
        fillRamp (dest, numSamples, position);
        return true;
    };
    source.getCurrentEnd = [end] { return end; };
    source.ringCapacitySamples = 0;  // eingefroren — kein Überholschutz nötig
    return source;
}

/** Writer + Temp-Verzeichnis + Abschluss-Synchronisation (WaitableEvent —
    onJobFinished läuft laut Kontrakt auf dem Writer-Thread). */
struct WriterFixture
{
    WriterFixture()
        : directory (juce::File::getSpecialLocation (juce::File::tempDirectory)
                         .getChildFile ("ConduitCaptureWriterTests")
                         .getChildFile (juce::Uuid().toString()))
    {
        directory.createDirectory();
        writer.onJobFinished = [this] (const CaptureWriter::Report& finishedReport)
        {
            report = finishedReport;
            done.signal();
        };
    }

    ~WriterFixture()
    {
        writer.stopThread (10'000);
        directory.deleteRecursively();
    }

    bool runJob (CaptureWriter::Job job, int timeoutMs = 15'000)
    {
        job.directory = directory;
        writer.enqueueJob (std::move (job));
        return done.wait (timeoutMs);
    }

    juce::File directory;
    CaptureWriter writer;
    juce::WaitableEvent done;
    CaptureWriter::Report report;
};

/** Liest eine Mono-WAV vollständig zurück; nullptr-Reader → leerer Buffer. */
juce::AudioBuffer<float> readWavFile (const juce::File& file, juce::String& timeReference)
{
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader (
        format.createReaderFor (file.createInputStream().release(), true));

    juce::AudioBuffer<float> buffer;
    if (reader == nullptr)
        return buffer;

    timeReference = reader->metadataValues.getValue (juce::WavAudioFormat::bwavTimeReference, "");
    buffer.setSize (1, static_cast<int> (reader->lengthInSamples));
    reader->read (&buffer, 0, static_cast<int> (reader->lengthInSamples), 0, true, false);
    return buffer;
}

/** Kanal-Prüfstand wie in CaptureBufferTests: spielt den Audio Thread
    (process VOR PreRoll-write — Reihenfolge des CaptureService-Taps). */
struct ChannelHarness
{
    ChannelHarness (int preRollCapacity, int segmentSamples, int budget, int blockSize)
        : blockSizeSamples (blockSize)
    {
        preRoll.prepare (preRollCapacity);
        pool.prepare (segmentSamples, 2, 1);
        channel.prepare (preRoll, pool, budget);
        block.resize (static_cast<size_t> (blockSize));
    }

    void feedBlock()
    {
        fillRamp (block.data(), blockSizeSamples, position);
        channel.process (block.data(), blockSizeSamples, position);
        preRoll.write (block.data(), blockSizeSamples, position);
        position += static_cast<std::uint64_t> (blockSizeSamples);
    }

    void feedBlocks (int count)
    {
        for (int i = 0; i < count; ++i)
            feedBlock();
    }

    PreRollBuffer preRoll;
    BufferPool pool;
    CaptureChannel channel;
    std::vector<float> block;
    int blockSizeSamples;
    std::uint64_t position = 0;
};

} // namespace

//==============================================================================
TEST_CASE ("CaptureWriter: Alignment-Helfer sind pur und korrekt", "[capture][writer]")
{
    SECTION ("exportStart ist das Minimum aller Kanal-Starts")
    {
        std::vector<CaptureWriter::Task> tasks (3);
        tasks[0].startPosition = 4000;
        tasks[1].startPosition = 1500;
        tasks[2].startPosition = 9000;

        CHECK (CaptureWriter::computeExportStart (tasks) == 1500);
        CHECK (CaptureWriter::computeExportStart ({}) == 0);
    }

    SECTION ("padSamples = Kanal-Start minus exportStart, nie negativ")
    {
        CHECK (CaptureWriter::computePadSamples (4000, 1500) == 2500);
        CHECK (CaptureWriter::computePadSamples (1500, 1500) == 0);
        CHECK (CaptureWriter::computePadSamples (1000, 1500) == 0);  // defensiv
    }

    SECTION ("Dateiname folgt {timestamp}_{track}_{take}.wav")
    {
        CHECK (CaptureWriter::makeFileName ("2026-06-12_153055", "in3", 7)
               == "2026-06-12_153055_in3_7.wav");
    }
}

//==============================================================================
TEST_CASE ("CaptureWriter: Padding-Berechnung und BWF-TimeReference im File",
           "[capture][writer]")
{
    WriterFixture fx;

    // Zwei versetzt gestartete Kanäle — Kanal B beginnt 2500 Samples später
    constexpr std::uint64_t startA = 1000, endA = 9000;
    constexpr std::uint64_t startB = 3500, endB = 10000;

    CaptureWriter::Job job;
    job.sampleRate = 48000.0;
    job.bitDepth   = 24;
    job.takeNumber = 1;

    CaptureWriter::Task taskA;
    taskA.source = makeFrozenRampSource (startA, endA);
    taskA.startPosition = startA;
    taskA.endPosition   = endA;
    taskA.trackName     = "in1";
    taskA.channelIndex  = 0;
    job.tasks.push_back (std::move (taskA));

    CaptureWriter::Task taskB;
    taskB.source = makeFrozenRampSource (startB, endB);
    taskB.startPosition = startB;
    taskB.endPosition   = endB;
    taskB.trackName     = "in2";
    taskB.channelIndex  = 1;
    job.tasks.push_back (std::move (taskB));

    REQUIRE (fx.runJob (std::move (job)));
    REQUIRE (fx.report.numSucceeded == 2);
    CHECK (fx.report.numFailed == 0);
    CHECK (fx.report.exportStart == startA);

    juce::String timeRefA, timeRefB;
    const auto bufferA = readWavFile (fx.report.tasks[0].file, timeRefA);
    const auto bufferB = readWavFile (fx.report.tasks[1].file, timeRefB);

    // Beide Dateien beginnen am selben absoluten Sample (exportStart)
    CHECK (timeRefA == juce::String (startA));
    CHECK (timeRefB == juce::String (startA));

    // Längen: A ohne Padding, B mit 2500 Samples Stille vorweg
    REQUIRE (bufferA.getNumSamples() == static_cast<int> (endA - startA));
    REQUIRE (bufferB.getNumSamples() == static_cast<int> (2500 + (endB - startB)));

    // Padding ist exakt Null (Stille übersteht jede Quantisierung)
    for (int i = 0; i < 2500; ++i)
        REQUIRE (juce::exactlyEqual (bufferB.getSample (0, i), 0.0f));

    // Inhalt hinter dem Padding: Rampe ab startB (24-Bit-Toleranz)
    for (int i = 0; i < 64; ++i)
        REQUIRE (std::abs (bufferB.getSample (0, 2500 + i)
                           - rampValue (startB + static_cast<std::uint64_t> (i)))
                 < roundtripTolerance);

    // Kanal A: Rampe ab startA, Stichproben über die ganze Datei
    for (int i = 0; i < bufferA.getNumSamples(); i += 997)
        REQUIRE (std::abs (bufferA.getSample (0, i)
                           - rampValue (startA + static_cast<std::uint64_t> (i)))
                 < roundtripTolerance);
}

//==============================================================================
TEST_CASE ("CaptureWriter: Snapshot bei laufender Aufnahme endet exakt bei endPosition",
           "[capture][writer]")
{
    WriterFixture fx;

    constexpr int blockSize = 512;
    constexpr int segmentSamples = 1 << 16;
    ChannelHarness harness (4096, segmentSamples, 4096, blockSize);

    // Aufnahme starten: Pre-Roll füllen, Gate öffnen, Segment liefern,
    // Übernahme abschließen
    harness.feedBlocks (4);
    harness.channel.openGate (harness.position, 1024);
    harness.pool.service();
    harness.feedBlocks (8);
    REQUIRE (harness.channel.getState() == CaptureChannel::State::recording);
    REQUIRE (harness.channel.isTakeoverComplete());

    // Snapshot einfrieren — DANACH läuft die Aufnahme weiter
    const auto range = harness.channel.getReadableRange();
    REQUIRE (range.to > range.from);

    CaptureWriter::Job job;
    job.sampleRate = 48000.0;
    job.bitDepth   = 24;
    job.takeNumber = 3;

    CaptureWriter::Task task;
    auto* channel = &harness.channel;
    task.source.read = [channel] (std::uint64_t position, float* dest, int numSamples)
    { return channel->read (position, dest, numSamples); };
    task.source.getCurrentEnd = [channel] { return channel->getEndPosition(); };
    task.source.ringCapacitySamples = segmentSamples;
    task.startPosition = range.from;
    task.endPosition   = range.to;
    task.trackName     = "in1";
    task.channelIndex  = 0;
    job.tasks.push_back (std::move (task));

    job.directory = fx.directory;
    fx.writer.enqueueJob (std::move (job));

    // Producer schreibt WÄHREND des Exports weiter (SPSC: Leser hinter dem
    // Schreib-Cursor) — weit weg von der Überholschutz-Marge (Kapazität/8)
    for (int i = 0; i < 32; ++i)
        harness.feedBlock();

    REQUIRE (fx.done.wait (15'000));
    REQUIRE (fx.report.numSucceeded == 1);

    // Producer ist über das eingefrorene Ende hinaus — die Datei nicht
    CHECK (harness.channel.getEndPosition() > range.to);

    juce::String timeRef;
    const auto buffer = readWavFile (fx.report.tasks[0].file, timeRef);
    REQUIRE (buffer.getNumSamples() == static_cast<int> (range.to - range.from));
    CHECK (timeRef == juce::String (range.from));

    for (int i = 0; i < buffer.getNumSamples(); i += 311)
        REQUIRE (std::abs (buffer.getSample (0, i)
                           - rampValue (range.from + static_cast<std::uint64_t> (i)))
                 < roundtripTolerance);
}

//==============================================================================
TEST_CASE ("CaptureWriter: Fehler brechen nur den betroffenen Kanal ab",
           "[capture][writer]")
{
    WriterFixture fx;

    CaptureWriter::Job job;
    job.sampleRate = 48000.0;
    job.bitDepth   = 16;
    job.takeNumber = 1;

    CaptureWriter::Task good;
    good.source = makeFrozenRampSource (0, 4000);
    good.startPosition = 0;
    good.endPosition   = 4000;
    good.trackName     = "in1";
    good.channelIndex  = 0;
    job.tasks.push_back (std::move (good));

    CaptureWriter::Task bad;  // Quelle verweigert jede Lieferung (Overrun-Fall)
    bad.source.read = [] (std::uint64_t, float*, int) { return false; };
    bad.source.getCurrentEnd = [] { return std::uint64_t { 4000 }; };
    bad.source.ringCapacitySamples = 0;
    bad.startPosition = 0;
    bad.endPosition   = 4000;
    bad.trackName     = "in2";
    bad.channelIndex  = 1;
    job.tasks.push_back (std::move (bad));

    REQUIRE (fx.runJob (std::move (job)));
    CHECK (fx.report.numSucceeded == 1);
    CHECK (fx.report.numFailed == 1);

    for (const auto& result : fx.report.tasks)
    {
        if (result.trackName == "in1")
        {
            CHECK (result.success);
            CHECK (result.file.existsAsFile());
        }
        else
        {
            CHECK_FALSE (result.success);
            CHECK (result.error.isNotEmpty());
            CHECK_FALSE (result.file.existsAsFile());  // Abbruch löscht die Datei
        }
    }
}

//==============================================================================
TEST_CASE ("CaptureWriter: Überholschutz bricht ab, bevor der Wrap den Leser erreicht",
           "[capture][writer]")
{
    WriterFixture fx;

    CaptureWriter::Job job;
    job.sampleRate = 48000.0;
    job.bitDepth   = 24;
    job.takeNumber = 1;

    // Live-Ende liegt bereits eine ganze Ring-Kapazität vor dem Lese-Start —
    // Headroom unter der Marge, der Kanal muss sofort abbrechen
    CaptureWriter::Task task;
    task.source.read = [] (std::uint64_t pos, float* dest, int numSamples)
    {
        fillRamp (dest, numSamples, pos);
        return true;
    };
    task.source.getCurrentEnd = [] { return std::uint64_t { 20'000 }; };
    task.source.ringCapacitySamples = 8192;
    task.startPosition = 0;
    task.endPosition   = 4000;
    task.trackName     = "in1";
    task.channelIndex  = 0;
    job.tasks.push_back (std::move (task));

    REQUIRE (fx.runJob (std::move (job)));
    CHECK (fx.report.numFailed == 1);
    CHECK (fx.report.tasks[0].error.containsIgnoreCase ("Overrun"));
}

//==============================================================================
TEST_CASE ("CaptureChannel: Export-Halte-Protokoll schiebt Freigaben auf",
           "[capture][writer]")
{
    ChannelHarness harness (2048, 1 << 14, 2048, 256);

    // Aufnahme aufbauen und in den Zustand held bringen
    harness.feedBlocks (4);
    harness.channel.openGate (harness.position, 512);
    harness.pool.service();
    harness.feedBlocks (4);
    REQUIRE (harness.channel.getState() == CaptureChannel::State::recording);
    harness.channel.closeGate (harness.position);
    REQUIRE (harness.channel.getState() == CaptureChannel::State::held);

    const auto range = harness.channel.getReadableRange();
    REQUIRE (range.to > range.from);

    // Export-Leser anmelden, dann Freigabe anfordern (RAM-Wächter-Pfad)
    REQUIRE (harness.channel.tryBeginExportRead());
    harness.channel.requestRelease();
    harness.feedBlock();

    // Freigabe ist aufgeschoben: Zustand bleibt held, Lesen funktioniert
    CHECK (harness.channel.getState() == CaptureChannel::State::held);
    std::vector<float> samples (64);
    CHECK (harness.channel.read (range.from, samples.data(), 64));

    // Solange die Freigabe ansteht, werden neue Leser abgewiesen (Barriere)
    CHECK_FALSE (harness.channel.tryBeginExportRead());

    // Leser fertig → der nächste Block holt die Freigabe nach
    harness.channel.endExportRead();
    harness.feedBlock();
    CHECK (harness.channel.getState() == CaptureChannel::State::idle);
    CHECK_FALSE (harness.channel.read (range.from, samples.data(), 64));

    // Barriere ist wieder offen — neue Leser sind möglich (Kanal ist idle,
    // exportAll überspringt ihn dann anhand des Zustands)
    CHECK (harness.channel.tryBeginExportRead());
    harness.channel.endExportRead();
}
