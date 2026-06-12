#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include "Core/Capture/CaptureService.h"
#include "Core/Capture/CaptureSettings.h"

using Catch::Approx;
using conduit::CaptureSettings;

namespace
{

/** Persistenz in ein Temp-Verzeichnis statt in die echte Conduit.settings. */
struct TempSettingsFolder
{
    TempSettingsFolder()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitCaptureSettingsTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempSettingsFolder() { folder.deleteRecursively(); }

    [[nodiscard]] juce::PropertiesFile::Options options() const
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "ConduitCaptureSettingsTests";
        o.filenameSuffix  = ".settings";
        o.folderName      = folder.getFullPathName();  // absoluter Pfad
        return o;
    }

    juce::File folder;
};

/** Gemockter Service-Status für die Resize-Policy-Zustandslogik. */
struct MockBufferHost : conduit::ICaptureBufferHost
{
    [[nodiscard]] bool isAnyChannelActive() const override { return active; }
    void invalidateAllBuffers() override { ++invalidateCalls; }
    void reallocateBuffers() override    { ++reallocateCalls; }

    bool active = false;
    int invalidateCalls = 0;
    int reallocateCalls = 0;
};

} // namespace

//==============================================================================
TEST_CASE ("CaptureSettings: Defaults und Clamping der Ranges", "[capture]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;
    CaptureSettings settings (temp.options());

    SECTION ("Defaults")
    {
        REQUIRE (settings.getBufferMinutes()  == 15);
        REQUIRE (settings.getPreRollSeconds() == 60);
        REQUIRE (settings.getThresholdDb()    == Approx (-40.0f));
        REQUIRE (settings.getHoldMinutes()    == 10);
        REQUIRE (settings.getAutoCalibrate()  == true);
        REQUIRE (settings.getRamLimitGb()     == 3);
        REQUIRE (settings.getExportBitDepth() == 24);
        REQUIRE (settings.getExportDirectory()
                 == juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                        .getChildFile ("Conduit Captures"));
    }

    SECTION ("Clamping: thresholdDb -80…-20")
    {
        settings.setThresholdDb (-200.0f);
        REQUIRE (settings.getThresholdDb() == Approx (-80.0f));
        settings.setThresholdDb (-5.0f);
        REQUIRE (settings.getThresholdDb() == Approx (-20.0f));
    }

    SECTION ("Clamping: holdMinutes 1…30, ramLimitGb >= 1")
    {
        settings.setHoldMinutes (0);
        REQUIRE (settings.getHoldMinutes() == 1);
        settings.setHoldMinutes (99);
        REQUIRE (settings.getHoldMinutes() == 30);

        settings.setRamLimitGb (0);
        REQUIRE (settings.getRamLimitGb() == 1);
    }

    SECTION ("Clamping: bufferMinutes 5…30, preRollSeconds 10…120 (ohne Host = stille Übernahme)")
    {
        REQUIRE (settings.setBufferMinutes (1)  == CaptureSettings::ResizeOutcome::applied);
        REQUIRE (settings.getBufferMinutes() == 5);
        REQUIRE (settings.setBufferMinutes (99) == CaptureSettings::ResizeOutcome::applied);
        REQUIRE (settings.getBufferMinutes() == 30);

        REQUIRE (settings.setPreRollSeconds (5)   == CaptureSettings::ResizeOutcome::applied);
        REQUIRE (settings.getPreRollSeconds() == 10);
        REQUIRE (settings.setPreRollSeconds (999) == CaptureSettings::ResizeOutcome::applied);
        REQUIRE (settings.getPreRollSeconds() == 120);
    }

    SECTION ("exportBitDepth: nur 16/24/32, sonst Default")
    {
        settings.setExportBitDepth (16);
        REQUIRE (settings.getExportBitDepth() == 16);
        settings.setExportBitDepth (32);
        REQUIRE (settings.getExportBitDepth() == 32);
        settings.setExportBitDepth (23);
        REQUIRE (settings.getExportBitDepth() == 24);
    }
}

//==============================================================================
TEST_CASE ("CaptureSettings: Persistenz-Roundtrip via ApplicationProperties", "[capture]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;

    {
        CaptureSettings settings (temp.options());
        REQUIRE (settings.setBufferMinutes (20) == CaptureSettings::ResizeOutcome::applied);
        REQUIRE (settings.setPreRollSeconds (90) == CaptureSettings::ResizeOutcome::applied);
        settings.setThresholdDb (-55.5f);
        settings.setHoldMinutes (5);
        settings.setAutoCalibrate (false);
        settings.setRamLimitGb (4);
        settings.setExportBitDepth (32);
        settings.setExportDirectory (temp.folder.getChildFile ("Exports"));
        settings.flush();
    }

    CaptureSettings reloaded (temp.options());
    REQUIRE (reloaded.getBufferMinutes()  == 20);
    REQUIRE (reloaded.getPreRollSeconds() == 90);
    REQUIRE (reloaded.getThresholdDb()    == Approx (-55.5f));
    REQUIRE (reloaded.getHoldMinutes()    == 5);
    REQUIRE (reloaded.getAutoCalibrate()  == false);
    REQUIRE (reloaded.getRamLimitGb()     == 4);
    REQUIRE (reloaded.getExportBitDepth() == 32);
    REQUIRE (reloaded.getExportDirectory() == temp.folder.getChildFile ("Exports"));
}

//==============================================================================
TEST_CASE ("CaptureSettings: Resize-Policy — kein Kanal aktiv = stille Reallokation", "[capture]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;
    CaptureSettings settings (temp.options());

    MockBufferHost host;
    settings.setBufferHost (&host);

    bool callbackFired = false;
    settings.onPendingResize = [&callbackFired] (const CaptureSettings::PendingResizeRequest&)
    {
        callbackFired = true;
    };

    REQUIRE (settings.setBufferMinutes (20) == CaptureSettings::ResizeOutcome::applied);
    REQUIRE (settings.getBufferMinutes() == 20);
    REQUIRE (host.reallocateCalls == 1);
    REQUIRE (host.invalidateCalls == 0);   // stille Übernahme verwirft nichts
    REQUIRE_FALSE (callbackFired);
    REQUIRE_FALSE (settings.hasPendingResize());

    // Unveränderter Wert: kein erneuter Realloc
    REQUIRE (settings.setBufferMinutes (20) == CaptureSettings::ResizeOutcome::applied);
    REQUIRE (host.reallocateCalls == 1);
}

//==============================================================================
TEST_CASE ("CaptureSettings: Resize-Policy — Kanal aktiv = Pending + Confirm/Cancel", "[capture]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;
    CaptureSettings settings (temp.options());

    MockBufferHost host;
    host.active = true;  // Gate offen oder "held"
    settings.setBufferHost (&host);

    std::vector<CaptureSettings::PendingResizeRequest> requests;
    settings.onPendingResize = [&requests] (const CaptureSettings::PendingResizeRequest& request)
    {
        requests.push_back (request);
    };

    SECTION ("Aktiv: Wert wird NICHT übernommen, Callback feuert")
    {
        REQUIRE (settings.setBufferMinutes (25) == CaptureSettings::ResizeOutcome::pendingConfirmation);
        REQUIRE (settings.getBufferMinutes() == 15);  // unverändert
        REQUIRE (host.reallocateCalls == 0);
        REQUIRE (host.invalidateCalls == 0);

        REQUIRE (requests.size() == 1);
        REQUIRE (requests[0].field == CaptureSettings::PendingResizeRequest::Field::bufferMinutes);
        REQUIRE (requests[0].currentValue == 15);
        REQUIRE (requests[0].requestedValue == 25);
        REQUIRE (settings.hasPendingResize());
    }

    SECTION ("Cancel: Anfrage verworfen, nichts invalidiert")
    {
        settings.setBufferMinutes (25);
        settings.cancelPendingResize();

        REQUIRE_FALSE (settings.hasPendingResize());
        REQUIRE (settings.getBufferMinutes() == 15);
        REQUIRE (host.invalidateCalls == 0);
        REQUIRE (host.reallocateCalls == 0);
    }

    SECTION ("Confirm: invalidieren → übernehmen → reallokieren")
    {
        settings.setPreRollSeconds (90);
        REQUIRE (settings.getPreRollSeconds() == 60);  // noch nicht übernommen

        settings.confirmPendingResize();

        REQUIRE (host.invalidateCalls == 1);
        REQUIRE (settings.getPreRollSeconds() == 90);
        REQUIRE (host.reallocateCalls == 1);
        REQUIRE_FALSE (settings.hasPendingResize());

        // Doppel-Confirm ist ein No-op
        settings.confirmPendingResize();
        REQUIRE (host.invalidateCalls == 1);
    }

    SECTION ("Latest wins: zweite Anfrage ersetzt die erste")
    {
        settings.setBufferMinutes (20);
        settings.setBufferMinutes (25);

        REQUIRE (requests.size() == 2);
        const auto pending = settings.getPendingResize();
        REQUIRE (pending.has_value());
        REQUIRE (pending->requestedValue == 25);

        settings.confirmPendingResize();
        REQUIRE (settings.getBufferMinutes() == 25);
    }

    SECTION ("Gate schließt zwischenzeitlich: nächster Versuch geht still durch")
    {
        settings.setBufferMinutes (25);
        settings.cancelPendingResize();

        host.active = false;
        REQUIRE (settings.setBufferMinutes (25) == CaptureSettings::ResizeOutcome::applied);
        REQUIRE (settings.getBufferMinutes() == 25);
        REQUIRE (host.invalidateCalls == 0);  // inaktiv → nichts zu verwerfen
        REQUIRE (host.reallocateCalls == 1);
    }
}

//==============================================================================
TEST_CASE ("CaptureService: Ring-Dimensionierung aus den Settings", "[capture]")
{
    SECTION ("computeRingCapacitySamples — pur, ohne Allokation")
    {
        using conduit::CaptureService;

        // 15 min @ 48 kHz, 2 Kanäle, 3 GB: RAM-Limit greift nicht
        REQUIRE (CaptureService::computeRingCapacitySamples (15, 48000.0, 2, 3) == 43'200'000);

        // 64 Kanäle: 3 GiB / (64 ch * 4 B) = 12'582'912 Samples — Limit greift
        REQUIRE (CaptureService::computeRingCapacitySamples (15, 48000.0, 64, 3) == 12'582'912);

        // Defensive Nullfälle
        REQUIRE (CaptureService::computeRingCapacitySamples (0, 48000.0, 2, 3)  == 0);
        REQUIRE (CaptureService::computeRingCapacitySamples (15, 0.0, 2, 3)     == 0);
        REQUIRE (CaptureService::computeRingCapacitySamples (15, 48000.0, 0, 3) == 0);
        REQUIRE (CaptureService::computeRingCapacitySamples (15, 48000.0, 2, 0) == 0);
    }

    SECTION ("prepare() allokiert anhand der Settings — niedrige Samplerate hält den Test klein")
    {
        juce::ScopedJuceInitialiser_GUI juceRuntime;
        TempSettingsFolder temp;
        CaptureSettings settings (temp.options());
        conduit::CaptureService service (settings);

        service.prepare (1000.0, 32, 2);  // 15 min * 60 s * 1000 Hz
        REQUIRE (service.getRingCapacitySamples() == 900'000);
        REQUIRE (service.getRingNumChannels() == 2);

        // Resize ohne aktiven Kanal: reallocateBuffers() über die Policy
        settings.setBufferHost (&service);
        REQUIRE (settings.setBufferMinutes (5) == CaptureSettings::ResizeOutcome::applied);
        REQUIRE (service.getRingCapacitySamples() == 300'000);
    }
}
