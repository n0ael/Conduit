#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/LooperSettings.h"
#include "UI/LooperPage.h"
#include "UI/LooperSettingsMenu.h"

using Catch::Approx;

//==============================================================================
TEST_CASE ("LooperPage (M6): Panel-Struktur folgt setLooperCount", "[looper][ui]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;

    conduit::LooperPage page;
    page.setBounds (0, 0, 1400, 700);

    int rebuilds = 0;
    page.onPanelsChanged = [&] { ++rebuilds; };

    REQUIRE (page.getLooperCount() == 1);

    page.setLooperCount (3);
    REQUIRE (page.getLooperCount() == 3);
    REQUIRE (rebuilds == 1);

    // Clamps 1..4; No-op feuert keinen Rebuild
    page.setLooperCount (9);
    REQUIRE (page.getLooperCount() == 4);
    page.setLooperCount (4);
    REQUIRE (rebuilds == 2);

    page.setLooperCount (0);
    REQUIRE (page.getLooperCount() == 1);
}

TEST_CASE ("LooperPanel: Quellen-Auswahl und Track-Struktur", "[looper][ui]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;

    conduit::LooperPanel panel { 1 };
    panel.setBounds (0, 0, 460, 620);

    const std::vector<conduit::LooperPanel::Source> sources = {
        { "master", "Master" },
        { "hw:0",   "In 1 / In 2" },
    };

    juce::String selectedKey;
    panel.onSourceSelected = [&] (const juce::String& key) { selectedKey = key; };

    panel.setSources (sources, "hw:0");
    REQUIRE (panel.getSourceCombo().getSelectedItemIndex() == 1);
    REQUIRE (selectedKey.isEmpty());   // Anzeige ≠ User-Klick

    panel.getSourceCombo().setSelectedItemIndex (0, juce::sendNotificationSync);
    REQUIRE (selectedKey == "master");

    // Tracks 1..4, „+"-Kachel verschwindet bei 4
    REQUIRE (panel.getTrackCount() == 1);
    panel.setTrackCount (4);
    REQUIRE (panel.getTrackCount() == 4);
    REQUIRE_FALSE (panel.getAddTrackTile().isVisible());
    panel.setTrackCount (2);
    REQUIRE (panel.getAddTrackTile().isVisible());
}

TEST_CASE ("LooperPanel: Quellen-Menü mit Link-Gruppen — Separatoren, Farben, stabile IDs", "[looper][ui]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;

    conduit::LooperPanel panel { 1 };
    panel.setBounds (0, 0, 460, 620);

    const auto orange = juce::Colour (0xffffa726);
    const auto green  = juce::Colour (0xff66bb6a);
    const auto cyan   = juce::Colour (0xff4dd0e1);

    // Lokale Quellen | Link-Peer "Live" | Link-Peer "M4L" (je Separator davor)
    const std::vector<conduit::LooperPanel::Source> sources = {
        { "master",     "Master",       juce::Colour(), false },
        { "hw:0",       "In 1 / In 2",  orange,         false },
        { "tap:live_a", "Live / Drums", green,          true  },
        { "tap:live_b", "Live / Bass",  green,          false },
        { "tap:m4l",    "M4L / Synth",  cyan,           true  },
    };

    juce::String selectedKey;
    panel.onSourceSelected = [&] (const juce::String& key) { selectedKey = key; };

    panel.setSources (sources, "tap:live_a");
    auto& combo = panel.getSourceCombo();

    SECTION ("Separatoren strukturieren das Menü, ohne Items zu verschieben")
    {
        REQUIRE (combo.getNumItems() == 5);   // Separatoren zählen nicht

        int separators = 0;
        for (juce::PopupMenu::MenuItemIterator iterator (*combo.getRootMenu()); iterator.next();)
            if (iterator.getItem().isSeparator)
                ++separators;
        REQUIRE (separators == 2);

        // Auswahl über den Key gefunden — Item-ID = Quell-Index + 1
        REQUIRE (combo.getSelectedId() == 3);
        REQUIRE (selectedKey.isEmpty());   // Anzeige ≠ User-Klick
    }

    SECTION ("Auswahl-Wechsel meldet trotz Separatoren den richtigen Key")
    {
        combo.setSelectedId (5, juce::sendNotificationSync);
        REQUIRE (selectedKey == "tap:m4l");

        combo.setSelectedId (1, juce::sendNotificationSync);
        REQUIRE (selectedKey == "master");
    }

    SECTION ("Menü-Einträge und Combo-Text tragen die Quellfarbe")
    {
        // Item-Farben im RootMenu (Position: Master, hw, SEP, live_a, …)
        juce::Array<juce::Colour> itemColours;
        for (juce::PopupMenu::MenuItemIterator iterator (*combo.getRootMenu()); iterator.next();)
            if (! iterator.getItem().isSeparator)
                itemColours.add (iterator.getItem().colour);

        REQUIRE (itemColours.size() == 5);
        REQUIRE (itemColours[1] == orange);
        REQUIRE (itemColours[2] == green);
        REQUIRE (itemColours[4] == cyan);

        // Combo-Text: Farbe der Auswahl; farblose Quelle → Standard-Text
        REQUIRE (combo.findColour (juce::ComboBox::textColourId) == green);

        combo.setSelectedId (1, juce::sendNotificationSync);
        REQUIRE (combo.findColour (juce::ComboBox::textColourId)
                 == conduit::push::colours::text);
    }
}

TEST_CASE ("LooperTrackStrip: Hooks liefern Track-lokale Indizes und Werte", "[looper][ui]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;

    conduit::LooperTrackStrip strip { 2 };
    strip.setBounds (0, 0, 120, 520);

    SECTION ("Mute/Solo togglen über den externen Zustand")
    {
        bool muted = false, solo = false;
        strip.onMuteToggled = [&] (bool m) { muted = m; };
        strip.onSoloToggled = [&] (bool s) { solo = s; };

        strip.getMuteTile().onClick();
        
        REQUIRE (muted);

        strip.setMute (true);   // Editor bestätigt → nächster Klick toggelt aus
        strip.getMuteTile().onClick();
        
        REQUIRE_FALSE (muted);

        strip.getSoloTile().onClick();
        
        REQUIRE (solo);
    }

    SECTION ("Slot-Taps melden den Slot-Index; sichtbare Zahl folgt dem Setter")
    {
        int tapped = -1;
        strip.onSlotTapped = [&] (int slot) { tapped = slot; };

        strip.setVisibleSlots (6);
        REQUIRE (strip.getVisibleSlots() == 6);

        strip.getSlotCell (4).onTap();
        REQUIRE (tapped == 4);

        strip.setVisibleSlots (12);
        REQUIRE (strip.getVisibleSlots() == 12);
        strip.getSlotCell (11).onTap();
        REQUIRE (tapped == 11);
    }

    SECTION ("Slot-Zellen-Zustand: Setter idempotent, Zustände sichtbar")
    {
        auto& cell = strip.getSlotCell (0);

        conduit::LooperSlotCell::State state;
        state.hasClip = true;
        state.playing = true;
        state.progress01 = 0.5f;
        state.label = "Clip 1 · 4 Bars";
        state.rateBadge = juce::String::fromUTF8 ("0.50×");
        cell.setState (state);

        REQUIRE (cell.getState().playing);
        REQUIRE (cell.getState().rateBadge.isNotEmpty());

        state.playing = false;
        cell.setState (state);
        REQUIRE_FALSE (cell.getState().playing);
    }
}

TEST_CASE ("LooperClipControlsRow: Dispatch nur mit Aktiv-Clip, VARI-Mapping", "[looper][ui]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;

    conduit::LooperClipControlsRow row;
    row.setBounds (0, 0, 640, 58);

    int doubled = 0, halved = 0, reversed = 0, synced = 0, cycles = 0;
    double lastRate = 1.0;
    bool lastRaster = false;

    row.onDoubleLength = [&] { ++doubled; };
    row.onHalveLength = [&] { ++halved; };
    row.onReverseToggled = [&] { ++reversed; };
    row.onResetWithSync = [&] { ++synced; };
    row.onTargetCycle = [&] { ++cycles; };
    row.onRateChanged = [&] (double rate) { lastRate = rate; };
    row.onRasterToggled = [&] (bool quantized) { lastRaster = quantized; };

    SECTION ("Ohne Aktiv-Clip sind die Clip-Controls wirkungslos (Übergabe §3)")
    {
        row.setClipControlsEnabled (false);
        row.getDoubleTile().onClick();
        row.getHalveTile().onClick();
        row.getReverseTile().onClick();
        row.getSyncTile().onClick();
        
        REQUIRE (doubled + halved + reversed + synced == 0);

        // Raster-Button bleibt bedienbar (Track-Eigenschaft, kein Clip nötig)
        row.getRasterTile().onClick();
        
        REQUIRE (lastRaster);
    }

    SECTION ("Mit Aktiv-Clip feuern die Hooks")
    {
        row.setClipControlsEnabled (true);
        row.getDoubleTile().onClick();
        row.getHalveTile().onClick();
        row.getReverseTile().onClick();
        row.getSyncTile().onClick();
        
        REQUIRE (doubled == 1);
        REQUIRE (halved == 1);
        REQUIRE (reversed == 1);
        REQUIRE (synced == 1);
    }

    SECTION ("VARI-Knob: Oktav-Mapping, Detent bei 1×, Rastung via snapFunction")
    {
        row.setClipControlsEnabled (true);

        // −1 Oktave → Rate 0.5
        row.getVariKnob().setValue (-1.0, juce::sendNotificationSync);
        REQUIRE (lastRate == Approx (0.5));

        // Detent: kleine Auslenkung fällt auf 1×
        row.getVariKnob().setValue (0.05, juce::sendNotificationSync);
        REQUIRE (lastRate == Approx (1.0));

        // Gerastert + Halbton-Snap: 0.30 Oktaven → 4/12 Oktaven
        row.setRasterQuantized (true);
        row.snapFunction = [] (double octaves) { return conduit::looperui::snapToSemitones (octaves); };
        row.getVariKnob().setValue (0.30, juce::sendNotificationSync);
        REQUIRE (lastRate == Approx (std::pow (2.0, 4.0 / 12.0)).epsilon (1.0e-6));
    }

    SECTION ("Pure Mapping-Helfer")
    {
        using namespace conduit::looperui;
        REQUIRE (rateFromOctaves (octavesFromRate (0.71)) == Approx (0.71));
        REQUIRE (rateFromOctaves (2.0) == Approx (4.0));
        REQUIRE (rateFromOctaves (-2.0) == Approx (0.25));
        REQUIRE (applyDetent (0.05) == Approx (0.0));
        REQUIRE (applyDetent (0.2) == Approx (0.2));
        REQUIRE (snapToSemitones (0.13) == Approx (2.0 / 12.0));
    }
}

TEST_CASE ("LooperSettingsMenu: Controls schreiben in die LooperSettings", "[looper][ui]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;

    const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("ConduitLooperMenuTests")
                            .getChildFile (juce::Uuid().toString());
    folder.createDirectory();

    juce::PropertiesFile::Options options;
    options.applicationName = "LooperMenuTests";
    options.filenameSuffix  = ".settings";
    options.folderName      = folder.getFullPathName();

    {
        conduit::LooperSettings settings { options };
        conduit::LooperSettingsMenu menu { settings };

        menu.getQuantCombo().setSelectedId ((int) conduit::LaunchQuant::off + 1,
                                            juce::sendNotificationSync);
        REQUIRE (settings.getLaunchQuant() == conduit::LaunchQuant::off);

        menu.getTapModeCombo().setSelectedId (2, juce::sendNotificationSync);
        REQUIRE (settings.getTapMode() == conduit::LooperSettings::TapMode::toggleStop);

        menu.getSlotsCombo().setSelectedId (10, juce::sendNotificationSync);
        REQUIRE (settings.getVisibleSlots() == 10);

        menu.getDeleteLatchToggle().setToggleState (true, juce::dontSendNotification);
        menu.getDeleteLatchToggle().onClick();
        REQUIRE (settings.isDeleteLatchEnabled());

        menu.getAutoAdvanceToggle().setToggleState (false, juce::dontSendNotification);
        menu.getAutoAdvanceToggle().onClick();
        REQUIRE_FALSE (settings.isAutoAdvanceEnabled());
    }

    folder.deleteRecursively();
}

TEST_CASE ("LooperPage: Kopfzeile — Output-Paare, Spectrum, Hooks", "[looper][ui]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;

    conduit::LooperPage page;
    page.setBounds (0, 0, 1400, 700);

    SECTION ("Output-Paare: Anker vorausgewählt, OOB geclampt, Klick meldet Index")
    {
        const juce::StringArray pairs { "Out 1 / Out 2", "Out 3 / Out 4" };
        int selectedPair = -1;
        page.onOutputPairSelected = [&] (int pair) { selectedPair = pair; };

        page.setOutputPairs (pairs, 1);
        REQUIRE (page.getOutputCombo().getSelectedItemIndex() == 1);
        REQUIRE (selectedPair == -1);

        page.setOutputPairs (pairs, 7);   // OOB → geclampt
        REQUIRE (page.getOutputCombo().getSelectedItemIndex() == 1);

        page.getOutputCombo().setSelectedItemIndex (0, juce::sendNotificationSync);
        REQUIRE (selectedPair == 0);
    }

    SECTION ("Spectrum-Kachel schaltet alle Panel-Strips")
    {
        page.setLooperCount (2);
        page.setSpectrumView (true);
        REQUIRE (page.getSpectrumTile().isActive());

        bool toggled = false;
        page.onViewToggled = [&] (bool spectrum) { toggled = ! spectrum; };
        page.getSpectrumTile().onClick();
        
        REQUIRE (toggled);   // aktiver Zustand → Toggle meldet false
    }

    SECTION ("−/+/⚙/Stop-Hooks feuern")
    {
        int added = 0, removed = 0, settingsOpened = 0, stopped = 0;
        page.onAddLooper = [&] { ++added; };
        page.onRemoveLooper = [&] { ++removed; };
        page.onOpenSettings = [&] { ++settingsOpened; };
        page.onStop = [&] { ++stopped; };

        page.getAddLooperTile().onClick();
        page.getRemoveLooperTile().onClick();
        page.getSettingsTile().onClick();
        page.getStopTile().onClick();
        

        REQUIRE (added == 1);
        REQUIRE (removed == 1);
        REQUIRE (settingsOpened == 1);
        REQUIRE (stopped == 1);
    }
}
