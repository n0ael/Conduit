#include <catch2/catch_test_macros.hpp>

#include "Core/ChannelNames.h"

using conduit::ChannelNames;
using Direction = conduit::ChannelNames::Direction;

namespace
{

/** Persistenz in ein Temp-Verzeichnis statt in die echte Settings-Datei. */
struct TempSettingsFolder
{
    TempSettingsFolder()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitChannelNamesTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempSettingsFolder() { folder.deleteRecursively(); }

    [[nodiscard]] juce::PropertiesFile::Options options() const
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "ConduitChannelNamesTests";
        o.filenameSuffix  = ".settings";
        o.folderName      = folder.getFullPathName();  // absoluter Pfad
        return o;
    }

    juce::File folder;
};

} // namespace

//==============================================================================
TEST_CASE ("ChannelNames: Default-Fallbacks ohne Eintrag", "[channelnames]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;
    ChannelNames names (temp.options());

    SECTION ("Ohne Device-Kontext: In N / Out N")
    {
        REQUIRE (names.getLabel (Direction::input, 0)  == "In 1");
        REQUIRE (names.getLabel (Direction::output, 3) == "Out 4");
    }

    SECTION ("Gemeldeter Kanalname des Devices hat Vorrang vor In N")
    {
        names.setActiveDevice ("ES-3", { "ADAT 1", "ADAT 2" }, { "Main L" });

        REQUIRE (names.getLabel (Direction::input, 0)  == "ADAT 1");
        REQUIRE (names.getLabel (Direction::input, 1)  == "ADAT 2");
        REQUIRE (names.getLabel (Direction::output, 0) == "Main L");

        // Außerhalb der gemeldeten Liste: Index-Fallback
        REQUIRE (names.getLabel (Direction::input, 2)  == "In 3");
        REQUIRE (names.getLabel (Direction::output, 1) == "Out 2");
    }

    SECTION ("Leerer gemeldeter Name fällt auf In N zurück")
    {
        names.setActiveDevice ("ES-3", { "" }, {});
        REQUIRE (names.getLabel (Direction::input, 0) == "In 1");
    }
}

//==============================================================================
TEST_CASE ("ChannelNames: userLabel-Overrides und Richtungs-Trennung", "[channelnames]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;
    ChannelNames names (temp.options());
    names.setActiveDevice ("ES-3", { "ADAT 1" }, {});

    SECTION ("Override gewinnt gegen gemeldeten Namen")
    {
        names.setUserLabel (Direction::input, 0, "Kick");
        REQUIRE (names.getLabel (Direction::input, 0) == "Kick");
        REQUIRE (names.getUserLabel (Direction::input, 0) == "Kick");
    }

    SECTION ("Input und Output sind getrennte Keys")
    {
        names.setUserLabel (Direction::input, 0, "Kick");
        REQUIRE (names.getUserLabel (Direction::output, 0).isEmpty());
        REQUIRE (names.getLabel (Direction::output, 0) == "Out 1");
    }

    SECTION ("Leere Eingabe löscht den Eintrag — zurück zum Default")
    {
        names.setUserLabel (Direction::input, 0, "Kick");
        names.setUserLabel (Direction::input, 0, "   ");
        REQUIRE (names.getUserLabel (Direction::input, 0).isEmpty());
        REQUIRE (names.getLabel (Direction::input, 0) == "ADAT 1");
    }

    SECTION ("Label wird getrimmt und gekürzt")
    {
        names.setUserLabel (Direction::input, 0, "  Kick  ");
        REQUIRE (names.getUserLabel (Direction::input, 0) == "Kick");

        names.setUserLabel (Direction::input, 0, juce::String::repeatedString ("x", 200));
        REQUIRE (names.getUserLabel (Direction::input, 0).length()
                 == ChannelNames::maxLabelLength);
    }

    SECTION ("Ohne aktives Device ist setUserLabel ein No-op")
    {
        ChannelNames detached (temp.options());
        detached.setUserLabel (Direction::input, 0, "Kick");
        REQUIRE (detached.getUserLabel (Direction::input, 0).isEmpty());
    }
}

//==============================================================================
TEST_CASE ("ChannelNames: Device-Matching exakt / Prefix / kein Match (8.1)", "[channelnames]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;
    ChannelNames names (temp.options());

    names.setActiveDevice ("ES-3", {}, {});
    names.setUserLabel (Direction::input, 0, "Kick");

    SECTION ("Exakter Match")
    {
        names.setActiveDevice ("ES-3", {}, {});
        REQUIRE (names.getLabel (Direction::input, 0) == "Kick");
    }

    SECTION ("Prefix-Match: aktives Device trägt Suffix")
    {
        names.setActiveDevice ("ES-3 (2)", {}, {});
        REQUIRE (names.getLabel (Direction::input, 0) == "Kick");
    }

    SECTION ("Prefix-Match: gespeicherter Key trägt Suffix")
    {
        ChannelNames fresh (temp.options());
        fresh.setActiveDevice ("ES-3 (2)", {}, {});
        fresh.setUserLabel (Direction::input, 1, "Snare");

        fresh.setActiveDevice ("ES-3", {}, {});
        REQUIRE (fresh.getLabel (Direction::input, 1) == "Snare");
    }

    SECTION ("Kein Match → Default, fremdes Profil bleibt unberührt")
    {
        names.setActiveDevice ("Babyface", {}, {});
        REQUIRE (names.getLabel (Direction::input, 0) == "In 1");

        names.setActiveDevice ("ES-3", {}, {});
        REQUIRE (names.getLabel (Direction::input, 0) == "Kick");
    }

    SECTION ("Schreiben bei Prefix-Match aktualisiert das bestehende Profil")
    {
        names.setActiveDevice ("ES-3 (2)", {}, {});
        names.setUserLabel (Direction::input, 0, "Bassdrum");

        names.setActiveDevice ("ES-3", {}, {});
        REQUIRE (names.getLabel (Direction::input, 0) == "Bassdrum");
    }
}

//==============================================================================
TEST_CASE ("ChannelNames: stripDeviceSuffix", "[channelnames]")
{
    REQUIRE (ChannelNames::stripDeviceSuffix ("ES-3 (2)")  == "ES-3");
    REQUIRE (ChannelNames::stripDeviceSuffix ("ES-3 (12)") == "ES-3");
    REQUIRE (ChannelNames::stripDeviceSuffix ("ES-3")      == "ES-3");

    // Klammern ohne Zähler-Semantik bleiben Teil des Namens
    REQUIRE (ChannelNames::stripDeviceSuffix ("Scarlett (USB)") == "Scarlett (USB)");
    REQUIRE (ChannelNames::stripDeviceSuffix ("(2)")            == "(2)");
    REQUIRE (ChannelNames::stripDeviceSuffix ("ES-3 ()")        == "ES-3 ()");
}

//==============================================================================
TEST_CASE ("ChannelNames: sanitizeFileLabel für Dateinamen", "[channelnames]")
{
    SECTION ("Verbotene Zeichen werden ersetzt")
    {
        REQUIRE (ChannelNames::sanitizeFileLabel ("a/b\\c:d*e?f\"g<h>i|j", "x")
                 == "a_b_c_d_e_f_g_h_i_j");
        REQUIRE (ChannelNames::sanitizeFileLabel (juce::String ("tab\there"), "x")
                 == "tab_here");
    }

    SECTION ("Trim und Längen-Limit")
    {
        REQUIRE (ChannelNames::sanitizeFileLabel ("  Kick  ", "x") == "Kick");
        REQUIRE (ChannelNames::sanitizeFileLabel (juce::String::repeatedString ("y", 200), "x")
                     .length() == ChannelNames::maxLabelLength);
    }

    SECTION ("Leeres Ergebnis fällt auf den Fallback zurück")
    {
        REQUIRE (ChannelNames::sanitizeFileLabel ("", "in1")    == "in1");
        REQUIRE (ChannelNames::sanitizeFileLabel ("   ", "in1") == "in1");
    }
}

//==============================================================================
TEST_CASE ("ChannelNames: Persistenz-Roundtrip", "[channelnames]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;

    {
        ChannelNames names (temp.options());
        names.setActiveDevice ("ES-3", {}, {});
        names.setUserLabel (Direction::input, 0, "Kick");
        names.setUserLabel (Direction::output, 2, "CV Pitch");
        names.setImagePath (Direction::input, 0, "C:/bilder/kick.png");
        names.flush();
    }

    ChannelNames reloaded (temp.options());
    reloaded.setActiveDevice ("ES-3", {}, {});

    REQUIRE (reloaded.getUserLabel (Direction::input, 0)  == "Kick");
    REQUIRE (reloaded.getUserLabel (Direction::output, 2) == "CV Pitch");
    REQUIRE (reloaded.getImagePath (Direction::input, 0)  == "C:/bilder/kick.png");

    // Gelöschte Labels überleben den Roundtrip nicht
    reloaded.setUserLabel (Direction::input, 0, "");
    reloaded.flush();

    ChannelNames again (temp.options());
    again.setActiveDevice ("ES-3", {}, {});
    REQUIRE (again.getUserLabel (Direction::input, 0).isEmpty());
    REQUIRE (again.getUserLabel (Direction::output, 2) == "CV Pitch");
}
