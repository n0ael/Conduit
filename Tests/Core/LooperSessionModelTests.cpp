#include <cmath>
#include <functional>
#include <memory>

#include <catch2/catch_test_macros.hpp>

#include "Core/Looper/BarSampleAnchors.h"
#include "Core/Looper/LooperSessionModel.h"

using conduit::BarSampleAnchors;
using conduit::CaptureService;
using conduit::CaptureSettings;
using conduit::LooperBank;
using conduit::LooperSessionModel;

namespace
{

constexpr double testSampleRate = 48000.0;
constexpr int    blockSize = 480;

/** Braced-Init mit Komma zerlegt REQUIRE-Makros — Helper statt Initializer. */
[[nodiscard]] LooperSessionModel::SlotAddress makeAddr (int track, int slot) noexcept
{
    LooperSessionModel::SlotAddress address;
    address.track = track;
    address.slot = slot;
    return address;
}

struct TempCaptureSettings
{
    TempCaptureSettings()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitLooperSessionModelTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();

        juce::PropertiesFile::Options options;
        options.applicationName = "ConduitLooperSessionModelTests";
        options.filenameSuffix  = ".settings";
        options.folderName      = folder.getFullPathName();
        settings = std::make_unique<CaptureSettings> (options);
    }

    ~TempCaptureSettings()
    {
        settings.reset();
        folder.deleteRecursively();
    }

    juce::File folder;
    std::unique_ptr<CaptureSettings> settings;
};

/** Modell-Rig: Bank + Capture + Anker wie im BankRig, plus SessionModel. */
struct ModelRig
{
    ModelRig()
    {
        service.prepare (testSampleRate, blockSize, 2);
        bank.prepare (testSampleRate, blockSize);
        service.setChannelArmed (0, true);
        service.setChannelArmed (1, true);

        feedBlocks (3);
        service.runRamGuard();
        feedBars (2.5);
    }

    void feedBlocks (int blocks)
    {
        for (int b = 0; b < blocks; ++b)
        {
            const auto blockStart = service.getSampleClock().now();

            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = input.getWritePointer (ch);
                for (int i = 0; i < blockSize; ++i)
                    data[i] = 0.4f;
            }

            service.processInputTap (input, 2);

            conduit::ClockState clock;
            clock.bpm = 120.0;
            clock.beatAtBlockStart = beat;
            clock.sampleRate = testSampleRate;

            anchors.process (clock, blockStart, blockSize);

            output.clear();
            bank.process (output, 2, clock, blockStart, anchors);

            beat += clock.beatsPerSample() * blockSize;
        }
    }

    void feedBars (double bars) { feedBlocks ((int) std::lround (bars * 4.0 * 24000.0 / blockSize)); }

    [[nodiscard]] juce::Result commit (int looper, int bars = 1)
    {
        return model.commit (looper, bars, service, 0, 1, anchors);
    }

    /** Delete/Replace bis zur Freigabe durchpumpen. */
    void settle()
    {
        for (int i = 0; i < 6; ++i)
        {
            feedBlocks (1);
            bank.serviceMessageThread();
        }
    }

    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempCaptureSettings temp;
    CaptureService service { *temp.settings };
    BarSampleAnchors anchors;
    LooperBank bank;
    LooperSessionModel model { bank };
    juce::AudioBuffer<float> input  { 2, blockSize };
    juce::AudioBuffer<float> output { 2, blockSize };
    double beat = 0.0;
};

} // namespace

//==============================================================================
TEST_CASE ("LooperSessionModel: Target — Arm/Disarm/Toggle, nur leere Slots", "[looper]")
{
    ModelRig rig;

    REQUIRE_FALSE (rig.model.getTarget (0).isValid());

    rig.model.armTarget (0, 0, 2);
    REQUIRE (rig.model.getTarget (0) == makeAddr (0, 2));

    // Anderer Slot verschiebt das Target
    rig.model.armTarget (0, 0, 5);
    REQUIRE (rig.model.getTarget (0) == makeAddr (0, 5));

    // Derselbe Slot erneut = disarm
    rig.model.armTarget (0, 0, 5);
    REQUIRE_FALSE (rig.model.getTarget (0).isValid());

    // Belegter Slot wird nie Target
    rig.model.armTarget (0, 0, 0);
    REQUIRE (rig.commit (0).wasOk());
    rig.model.disarmTarget (0);
    rig.model.armTarget (0, 0, 0);
    REQUIRE_FALSE (rig.model.getTarget (0).isValid());
}

TEST_CASE ("LooperSessionModel: Commit — Target-Pflicht, Slot-Zuweisung, Auto-Advance", "[looper]")
{
    ModelRig rig;

    // Ohne Target: klarer Fehler (Statushinweis, Übergabe §4)
    const auto noTarget = rig.commit (0);
    REQUIRE (noTarget.failed());
    REQUIRE (noTarget.getErrorMessage().contains ("Target"));

    rig.model.armTarget (0, 0, 0);
    REQUIRE (rig.commit (0).wasOk());

    // Clip liegt im Slot, spielt, ist aktiv; Target rückte auf Slot 1
    REQUIRE (rig.model.clipAt (0, 0, 0) != nullptr);
    REQUIRE (rig.model.getPlayingSlot (0, 0) == 0);
    REQUIRE (rig.model.getActiveSlot (0) == makeAddr (0, 0));
    REQUIRE (rig.model.getTarget (0) == makeAddr (0, 1));

    // Auto-Advance aus: Target bleibt stehen → nächster Commit ÜBERSCHREIBT
    rig.model.setAutoAdvance (false);
    REQUIRE (rig.commit (0).wasOk());
    auto* first = rig.model.clipAt (0, 0, 1);
    REQUIRE (first != nullptr);
    REQUIRE (rig.model.getTarget (0) == makeAddr (0, 1));

    const auto ramBefore = rig.bank.getRamBytesUsed();
    REQUIRE (rig.commit (0).wasOk());
    REQUIRE (rig.model.clipAt (0, 0, 1) != first);   // alter Clip ersetzt

    rig.settle();
    REQUIRE (rig.bank.getRamBytesUsed() <= ramBefore);   // Overwrite hat freigegeben
}

TEST_CASE ("LooperSessionModel: Auto-Advance nur nach unten — voller Track verliert das Target", "[looper]")
{
    ModelRig rig;
    rig.model.armTarget (0, 0, LooperSessionModel::maxSlots - 1);

    REQUIRE (rig.commit (0).wasOk());

    // Letzter Slot committet → kein freier darunter → kein Target
    REQUIRE_FALSE (rig.model.getTarget (0).isValid());
}

TEST_CASE ("LooperSessionModel: TARGET-Kurzklick zykelt durch die Tracks", "[looper]")
{
    ModelRig rig;
    REQUIRE (rig.model.addTrack (0));
    REQUIRE (rig.model.addTrack (0));   // 3 Tracks

    rig.model.cycleTargetTrack (0);
    REQUIRE (rig.model.getTarget (0) == makeAddr (0, 0));

    rig.model.cycleTargetTrack (0);
    REQUIRE (rig.model.getTarget (0) == makeAddr (1, 0));

    rig.model.cycleTargetTrack (0);
    REQUIRE (rig.model.getTarget (0) == makeAddr (2, 0));

    rig.model.cycleTargetTrack (0);   // zyklisch zurück
    REQUIRE (rig.model.getTarget (0) == makeAddr (0, 0));
}

TEST_CASE ("LooperSessionModel: Launch/Stop/Delete — Slot-Bookkeeping", "[looper]")
{
    ModelRig rig;

    rig.model.armTarget (0, 0, 0);
    REQUIRE (rig.commit (0).wasOk());
    rig.model.armTarget (0, 0, 3);
    REQUIRE (rig.commit (0).wasOk());
    rig.settle();

    // Slot 3 spielt (letzter Commit); Slot 0 starten → ein Clip pro Track
    REQUIRE (rig.model.getPlayingSlot (0, 0) == 3);
    REQUIRE (rig.model.startSlot (0, 0, 0, 0.0).wasOk());
    REQUIRE (rig.model.getPlayingSlot (0, 0) == 0);
    REQUIRE (rig.model.getActiveSlot (0) == makeAddr (0, 0));

    // Leerer Slot: sauberer Fehler
    REQUIRE (rig.model.startSlot (0, 0, 7, 0.0).failed());
    REQUIRE (rig.model.retriggerSlot (0, 0, 7, 0.0).failed());

    // Stop räumt den spielenden Slot
    rig.model.stopTrack (0, 0, 0.0);
    REQUIRE (rig.model.getPlayingSlot (0, 0) == -1);

    // Delete räumt Slot + Referenzen; RAM konvergiert auf einen Clip
    rig.model.setActiveSlot (0, 0, 3);
    REQUIRE (rig.model.deleteSlot (0, 0, 3).wasOk());
    REQUIRE (rig.model.clipAt (0, 0, 3) == nullptr);
    REQUIRE_FALSE (rig.model.getActiveSlot (0).isValid());
    REQUIRE (rig.model.deleteSlot (0, 0, 3).failed());   // schon leer

    rig.settle();
    const auto oneClip = rig.bank.getRamBytesUsed();
    REQUIRE (oneClip > 0);
    REQUIRE (rig.model.clipAt (0, 0, 0) != nullptr);
}

TEST_CASE ("LooperSessionModel: Struktur — Looper/Track-Grenzen und Guards", "[looper]")
{
    ModelRig rig;

    REQUIRE (rig.model.getNumLoopers() == 1);
    REQUIRE (rig.model.removeLastLooper().failed());   // min. 1 bleibt

    REQUIRE (rig.model.addLooper());
    REQUIRE (rig.model.addLooper());
    REQUIRE (rig.model.addLooper());
    REQUIRE_FALSE (rig.model.addLooper());             // max. 4
    REQUIRE (rig.model.getNumLoopers() == 4);

    // Tracks: 1..4
    REQUIRE (rig.model.getNumTracks (0) == 1);
    REQUIRE (rig.model.addTrack (0));
    REQUIRE (rig.model.addTrack (0));
    REQUIRE (rig.model.addTrack (0));
    REQUIRE_FALSE (rig.model.addTrack (0));
    REQUIRE (rig.model.getNumTracks (0) == 4);

    SECTION ("Track entfernen: nur leer und gestoppt")
    {
        rig.model.armTarget (0, 3, 0);
        REQUIRE (rig.commit (0).wasOk());

        REQUIRE (rig.model.removeLastTrack (0).failed());   // enthält Clip

        REQUIRE (rig.model.deleteSlot (0, 3, 0).wasOk());
        rig.settle();
        REQUIRE (rig.model.removeLastTrack (0).wasOk());
        REQUIRE (rig.model.getNumTracks (0) == 3);
        REQUIRE (rig.model.removeLastTrack (0).wasOk());
        REQUIRE (rig.model.removeLastTrack (0).wasOk());
        REQUIRE (rig.model.removeLastTrack (0).failed());   // letzter bleibt
    }

    SECTION ("Looper schließen verwirft dessen Clips (looperHasClips fürs UI)")
    {
        rig.model.armTarget (3, 0, 0);
        REQUIRE (rig.commit (3).wasOk());
        REQUIRE (rig.model.looperHasClips (3));

        const auto ramWithClip = rig.bank.getRamBytesUsed();
        REQUIRE (rig.model.removeLastLooper().wasOk());
        REQUIRE (rig.model.getNumLoopers() == 3);

        rig.settle();
        REQUIRE (rig.bank.getRamBytesUsed() < ramWithClip);
    }
}

TEST_CASE ("LooperSessionModel: clearAllClips nach Bank-Prepare (Zombie-Schutz)", "[looper]")
{
    ModelRig rig;

    rig.model.armTarget (0, 0, 0);
    REQUIRE (rig.commit (0).wasOk());
    REQUIRE (rig.model.clipAt (0, 0, 0) != nullptr);

    // prepareToPlay-Pfad: Bank verwirft die Clips → Modell MUSS folgen
    // (Feld-Fund 05.07.2026: Zombie-Zelle nach Device-Restart)
    rig.bank.prepare (testSampleRate, blockSize);
    rig.model.clearAllClips();

    REQUIRE (rig.model.clipAt (0, 0, 0) == nullptr);
    REQUIRE (rig.model.getPlayingSlot (0, 0) == -1);
    REQUIRE_FALSE (rig.model.getTarget (0).isValid());
    REQUIRE_FALSE (rig.model.getActiveSlot (0).isValid());
}

TEST_CASE ("LooperSessionModel: Commits auf 4 Loopern parallel", "[looper]")
{
    ModelRig rig;
    while (rig.model.getNumLoopers() < 4)
        rig.model.addLooper();

    for (int l = 0; l < 4; ++l)
    {
        rig.model.armTarget (l, 0, 0);
        REQUIRE (rig.commit (l).wasOk());
    }

    rig.feedBlocks (4);
    // 4 Loops à 0.4 auf demselben Anker-Paar
    REQUIRE (std::abs (rig.output.getSample (0, 100) - 1.6f) < 0.02f);

    for (int l = 0; l < 4; ++l)
        REQUIRE (rig.model.getPlayingSlot (l, 0) == 0);
}
