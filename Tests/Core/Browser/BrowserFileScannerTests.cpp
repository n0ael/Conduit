#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/Browser/BrowserFileScanner.h"
#include "TestDispatcher.h"

namespace
{
struct ScannerRig
{
    ScannerRig()
    {
        folder.createDirectory();

        scanner.onScanComplete = [this] (const juce::String& scanId,
                                         std::vector<conduit::BrowserFileScanner::Entry> entries)
        {
            lastScanId  = scanId;
            lastEntries = std::move (entries);
            ++completedScans;
        };
    }

    ~ScannerRig() { folder.deleteRecursively(); }

    /** Startet den Scan und pumpt bis zum Ergebnis. */
    bool scanAndWait (const juce::String& scanId, const juce::String& wildcard,
                      bool readAudioMetadata)
    {
        const auto before = completedScans;
        scanner.scanAsync (scanId, folder, wildcard, readAudioMetadata);
        return dispatcher.pumpUntil ([this, before]
                                     { return completedScans > before; });
    }

    /** Mini-WAV (48 kHz, mono, 16 Bit, numSamples Stille) schreiben. */
    void writeWav (const juce::String& fileName, int numSamples)
    {
        juce::WavAudioFormat wavFormat;
        auto stream = folder.getChildFile (fileName).createOutputStream();
        REQUIRE (stream != nullptr);

        std::unique_ptr<juce::AudioFormatWriter> writer (
            wavFormat.createWriterFor (stream.release(), 48000.0, 1, 16, {}, 0));
        REQUIRE (writer != nullptr);

        juce::AudioBuffer<float> silence (1, numSamples);
        silence.clear();
        REQUIRE (writer->writeFromAudioSampleBuffer (silence, 0, numSamples));
    }

    juce::ScopedJuceInitialiser_GUI juceRuntime;
    juce::File folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("ConduitScannerTests")
                            .getChildFile (juce::Uuid().toString());
    juce::ThreadPool worker { juce::ThreadPoolOptions{}.withNumberOfThreads (1) };
    conduit::test::QueueDispatcher dispatcher;
    conduit::BrowserFileScanner scanner { worker, dispatcher.fn() };

    juce::String lastScanId;
    std::vector<conduit::BrowserFileScanner::Entry> lastEntries;
    int completedScans = 0;
};
} // namespace

//==============================================================================
TEST_CASE ("Datei-Scanner: WAV-Metadaten (Dauer/Format) aus dem Header",
           "[browser][scanner]")
{
    ScannerRig rig;
    rig.writeWav ("loop_a.wav", 24000);   // 0,5 s @ 48 kHz
    rig.writeWav ("loop_b.wav", 48000);   // 1,0 s

    REQUIRE (rig.scanAndWait ("loops", "*.wav", true));

    REQUIRE (rig.lastScanId == "loops");
    REQUIRE (rig.lastEntries.size() == 2);

    // alphabetisch sortiert
    REQUIRE (rig.lastEntries[0].name == "loop_a");
    REQUIRE (rig.lastEntries[0].durationSeconds == Catch::Approx (0.5));
    REQUIRE (rig.lastEntries[0].formatSummary == "48k / 16 Bit / mo");
    REQUIRE (rig.lastEntries[1].name == "loop_b");
    REQUIRE (rig.lastEntries[1].durationSeconds == Catch::Approx (1.0));
}

TEST_CASE ("Datei-Scanner: mtime-Cache überspringt unveränderte Dateien",
           "[browser][scanner]")
{
    ScannerRig rig;
    rig.writeWav ("take.wav", 24000);

    REQUIRE (rig.scanAndWait ("captures", "*.wav", true));
    const auto readsAfterFirst = rig.scanner.getMetadataReadCount();
    REQUIRE (readsAfterFirst == 1);

    // Rescan ohne Änderung: kein neuer Reader
    REQUIRE (rig.scanAndWait ("captures", "*.wav", true));
    REQUIRE (rig.scanner.getMetadataReadCount() == readsAfterFirst);
    REQUIRE (rig.lastEntries.size() == 1);
    REQUIRE (rig.lastEntries[0].formatSummary == "48k / 16 Bit / mo");
}

TEST_CASE ("Datei-Scanner: Wildcard filtert (*.conduit ohne Metadaten)",
           "[browser][scanner]")
{
    ScannerRig rig;
    rig.folder.getChildFile ("session.conduit").replaceWithText ("<CONDUIT/>");
    rig.folder.getChildFile ("notizen.txt").replaceWithText ("nicht listen");
    rig.writeWav ("loop.wav", 4800);

    REQUIRE (rig.scanAndWait ("projects", "*.conduit", false));

    REQUIRE (rig.lastEntries.size() == 1);
    REQUIRE (rig.lastEntries[0].name == "session");
    REQUIRE (rig.lastEntries[0].formatSummary.isEmpty());
    REQUIRE (rig.scanner.getMetadataReadCount() == 0);   // kein Audio-Read
}
