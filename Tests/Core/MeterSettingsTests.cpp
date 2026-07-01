#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/MeterSettings.h"

using Catch::Approx;
using conduit::MeterSettings;
using Mode = conduit::MeterSettings::ClipResetMode;

namespace
{

/** Persistenz in ein Temp-Verzeichnis statt in die echte Settings-Datei. */
struct TempMeterSettings
{
    TempMeterSettings()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitMeterSettingsTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempMeterSettings() { folder.deleteRecursively(); }

    [[nodiscard]] juce::PropertiesFile::Options options() const
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "ConduitMeterSettingsTests";
        o.filenameSuffix  = ".settings";
        o.folderName      = folder.getFullPathName();  // absoluter Pfad
        return o;
    }

    juce::File folder;
};

} // namespace

//==============================================================================
TEST_CASE ("MeterSettings: Default ist manuell, mappt auf 0 s Auto-Clear", "[metersettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempMeterSettings temp;
    MeterSettings settings (temp.options());

    REQUIRE (settings.getClipResetMode() == Mode::manual);
    REQUIRE (settings.getClipHoldSeconds() == Approx (0.0f));
}

TEST_CASE ("MeterSettings: automatic mappt auf autoClearSeconds", "[metersettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempMeterSettings temp;
    MeterSettings settings (temp.options());

    settings.setClipResetMode (Mode::automatic);
    REQUIRE (settings.getClipResetMode() == Mode::automatic);
    REQUIRE (settings.getClipHoldSeconds() == Approx (MeterSettings::autoClearSeconds));

    settings.setClipResetMode (Mode::manual);
    REQUIRE (settings.getClipHoldSeconds() == Approx (0.0f));
}

TEST_CASE ("MeterSettings: Modus übersteht den Roundtrip", "[metersettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempMeterSettings temp;

    {
        MeterSettings settings (temp.options());
        settings.setClipResetMode (Mode::automatic);
    }

    MeterSettings reloaded (temp.options());
    REQUIRE (reloaded.getClipResetMode() == Mode::automatic);
}

TEST_CASE ("MeterSettings: ChangeBroadcast bei echter Änderung", "[metersettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempMeterSettings temp;
    MeterSettings settings (temp.options());

    struct Counter : juce::ChangeListener
    {
        int calls = 0;
        void changeListenerCallback (juce::ChangeBroadcaster*) override { ++calls; }
    } counter;

    settings.addChangeListener (&counter);
    settings.setClipResetMode (Mode::automatic);   // Änderung → Broadcast
    settings.setClipResetMode (Mode::automatic);   // no-op → kein Broadcast
    settings.dispatchPendingMessages();            // synchrone Zustellung
    settings.removeChangeListener (&counter);

    REQUIRE (counter.calls == 1);
}
