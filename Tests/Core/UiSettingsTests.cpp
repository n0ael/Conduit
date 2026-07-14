#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/UiSettings.h"

using Catch::Approx;
using conduit::UiSettings;

namespace
{

/** Persistenz in ein Temp-Verzeichnis statt in die echte Settings-Datei
    (Muster TempMeterSettings). */
struct TempUiSettings
{
    TempUiSettings()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitUiSettingsTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempUiSettings() { folder.deleteRecursively(); }

    [[nodiscard]] juce::PropertiesFile::Options options() const
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "ConduitUiSettingsTests";
        o.filenameSuffix  = ".settings";
        o.folderName      = folder.getFullPathName();  // absoluter Pfad
        return o;
    }

    juce::File folder;
};

} // namespace

//==============================================================================
TEST_CASE ("UiSettings: Defaults — Scale 1.0, FontScale 1.0, Dev Mode aus", "[uisettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempUiSettings temp;
    UiSettings settings (temp.options());

    REQUIRE (settings.getUiScale() == Approx (1.0f));
    REQUIRE (settings.getFontScale() == Approx (1.0f));
    REQUIRE_FALSE (settings.isDevModeEnabled());
    REQUIRE (settings.isDspMeterEnabled());   // Default an (wie Abletons CPU-Meter)
}

TEST_CASE ("UiSettings: Setter clampen auf die erlaubten Bereiche", "[uisettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempUiSettings temp;
    UiSettings settings (temp.options());

    settings.setUiScale (3.0f);
    REQUIRE (settings.getUiScale() == Approx (UiSettings::maxUiScale));
    settings.setUiScale (0.1f);
    REQUIRE (settings.getUiScale() == Approx (UiSettings::minUiScale));

    settings.setFontScale (2.0f);
    REQUIRE (settings.getFontScale() == Approx (UiSettings::maxFontScale));
    settings.setFontScale (0.0f);
    REQUIRE (settings.getFontScale() == Approx (UiSettings::minFontScale));
}

TEST_CASE ("UiSettings: Werte überstehen den Roundtrip", "[uisettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempUiSettings temp;

    {
        UiSettings settings (temp.options());
        settings.setUiScale (1.5f);
        settings.setFontScale (1.2f);
        settings.setDevModeEnabled (true);
        settings.setDspMeterEnabled (false);   // gegen den Default
    }

    UiSettings reloaded (temp.options());
    REQUIRE (reloaded.getUiScale() == Approx (1.5f));
    REQUIRE (reloaded.getFontScale() == Approx (1.2f));
    REQUIRE (reloaded.isDevModeEnabled());
    REQUIRE_FALSE (reloaded.isDspMeterEnabled());
}

TEST_CASE ("UiSettings: defekte Datei wird beim Laden geclamped", "[uisettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempUiSettings temp;

    // Datei mit absurden Werten vorbereiten (handeditiert/defekt)
    {
        juce::PropertiesFile file (temp.options());
        file.setValue ("uiScale", 99.0);
        file.setValue ("fontScale", 0.01);
        file.save();
    }

    UiSettings settings (temp.options());
    REQUIRE (settings.getUiScale() == Approx (UiSettings::maxUiScale));
    REQUIRE (settings.getFontScale() == Approx (UiSettings::minFontScale));
}

TEST_CASE ("UiSettings: ChangeBroadcast nur bei echter Änderung", "[uisettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempUiSettings temp;
    UiSettings settings (temp.options());

    struct Counter : juce::ChangeListener
    {
        int calls = 0;
        void changeListenerCallback (juce::ChangeBroadcaster*) override { ++calls; }
    } counter;

    settings.addChangeListener (&counter);

    // Pro Block dispatchen — der ChangeBroadcaster koalesziert mehrere
    // sendChangeMessage vor der Zustellung zu EINEM Callback
    settings.setUiScale (1.5f);            // Änderung → Broadcast
    settings.setUiScale (1.5f);            // no-op (exactlyEqual) → kein Broadcast
    settings.dispatchPendingMessages();

    settings.setDevModeEnabled (true);     // Änderung → Broadcast
    settings.setDevModeEnabled (true);     // no-op
    settings.setFontScale (1.0f);          // no-op (Default) → kein Broadcast
    settings.dispatchPendingMessages();

    settings.removeChangeListener (&counter);
    REQUIRE (counter.calls == 2);
}

TEST_CASE ("UiSettings: softKeyboard — Plattform-Default + Persistenz", "[uisettings]")
{
    using conduit::UiSettings;
    TempUiSettings temp;

    {
        UiSettings settings (temp.options());

        // Plattform-Default: AN auf Linux (Kiosk/Touch), AUS auf Desktop
        REQUIRE (settings.isSoftKeyboardEnabled()
                     == UiSettings::defaultSoftKeyboardEnabled);
#if JUCE_LINUX
        REQUIRE (UiSettings::defaultSoftKeyboardEnabled);
#else
        REQUIRE_FALSE (UiSettings::defaultSoftKeyboardEnabled);
#endif

        settings.setSoftKeyboardEnabled (! UiSettings::defaultSoftKeyboardEnabled);
    }

    // Zweite Instanz liest den persistierten Wert
    UiSettings reloaded (temp.options());
    REQUIRE (reloaded.isSoftKeyboardEnabled()
                 == ! UiSettings::defaultSoftKeyboardEnabled);
}

//==============================================================================
// UI-Framerate (User-Regel 14.07.2026): uiFpsLimit + UiFramePacer-Gate

#include "UI/UiFramePacer.h"

TEST_CASE ("UiSettings: uiFpsLimit — Default 120, Clamp auf 120/60/30, Roundtrip", "[uisettings][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempUiSettings temp;

    {
        UiSettings settings (temp.options());
        REQUIRE (settings.getUiFpsLimit() == 120);   // Default = Nativ (max 120)

        settings.setUiFpsLimit (60);
        REQUIRE (settings.getUiFpsLimit() == 60);

        // Krumme Werte klemmen auf den naechsten erlaubten Modus.
        settings.setUiFpsLimit (144);
        REQUIRE (settings.getUiFpsLimit() == 120);
        settings.setUiFpsLimit (1);
        REQUIRE (settings.getUiFpsLimit() == 30);
    }

    UiSettings reloaded (temp.options());
    REQUIRE (reloaded.getUiFpsLimit() == 30);   // persistiert
}

TEST_CASE ("uiframe::shouldRunFrame: Nativ laeuft jeden VBlank, Limits drosseln", "[uisettings][midirig]")
{
    double last = 0.0;

    // Nativ (>= 120): jeder Frame laeuft.
    for (double t = 0.0; t < 100.0; t += 8.33)
        REQUIRE (conduit::uiframe::shouldRunFrame (t, last, 120));

    // 60-fps-Limit auf einem 120-Hz-Monitor (8.33-ms-Frames): jeder zweite.
    last = 0.0;
    int ran = 0;
    for (int frame = 1; frame <= 120; ++frame)
        if (conduit::uiframe::shouldRunFrame (frame * 8.333, last, 60))
            ++ran;
    REQUIRE (ran >= 55);
    REQUIRE (ran <= 65);

    // 60-fps-Limit auf einem 60-Hz-Monitor (16.67 ms + Jitter): kippt NICHT
    // auf 30 fps (1-ms-Toleranz).
    last = 0.0;
    ran = 0;
    for (int frame = 1; frame <= 60; ++frame)
    {
        const auto jitter = (frame % 2 == 0) ? 0.3 : -0.3;
        if (conduit::uiframe::shouldRunFrame (frame * 16.667 + jitter, last, 60))
            ++ran;
    }
    REQUIRE (ran >= 58);

    // 30-fps-Limit auf einem 60-Hz-Monitor: etwa jeder zweite Frame.
    last = 0.0;
    ran = 0;
    for (int frame = 1; frame <= 60; ++frame)
        if (conduit::uiframe::shouldRunFrame (frame * 16.667, last, 30))
            ++ran;
    REQUIRE (ran >= 28);
    REQUIRE (ran <= 32);
}
