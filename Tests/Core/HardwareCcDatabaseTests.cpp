#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Core/HardwareCcDatabase.h"

namespace grid = conduit::grid;

//==============================================================================
TEST_CASE ("HardwareCcDatabase::parse: liest Geraete + Parameter", "[hardwarecc]")
{
    const auto devices = grid::HardwareCcDatabase::parse (R"(
# Kommentar

[Test Synth]
1 = Mod Wheel
74 = Filter Cutoff

[Anderes Geraet]
7 = Volume
)");

    REQUIRE (devices.size() == 2);
    CHECK (devices[0].name == "Test Synth");
    REQUIRE (devices[0].params.size() == 2);
    CHECK (devices[0].params[0].cc == 1);
    CHECK (devices[0].params[0].name == "Mod Wheel");
    CHECK (devices[0].params[1].cc == 74);
    CHECK (devices[0].params[1].name == "Filter Cutoff");

    CHECK (devices[1].name == "Anderes Geraet");
    REQUIRE (devices[1].params.size() == 1);
    CHECK (devices[1].params[0].cc == 7);
}

TEST_CASE ("HardwareCcDatabase::parse: fehlertolerant bei kaputten Zeilen", "[hardwarecc]")
{
    const auto devices = grid::HardwareCcDatabase::parse (R"(
7 = Verwaist (kein Geraet davor -- wird ignoriert)

[Leer]

[Gueltig]
ohne Gleichheitszeichen
200 = Ausserhalb 0..127
74 = Cutoff
)");

    REQUIRE (devices.size() == 2);
    CHECK (devices[0].name == "Leer");
    CHECK (devices[0].params.empty());
    CHECK (devices[1].name == "Gueltig");
    REQUIRE (devices[1].params.size() == 1);
    CHECK (devices[1].params[0].cc == 74);
}

TEST_CASE ("HardwareCcDatabase::merge: overlay ergaenzt neue Geraete/Parameter", "[hardwarecc]")
{
    auto base = grid::HardwareCcDatabase::parse ("[Synth A]\n1 = Mod Wheel\n");
    const auto overlay = grid::HardwareCcDatabase::parse ("[Synth B]\n7 = Volume\n");

    const auto merged = grid::HardwareCcDatabase::merge (std::move (base), overlay);

    REQUIRE (merged.size() == 2);
    CHECK (merged[0].name == "Synth A");
    CHECK (merged[1].name == "Synth B");
}

TEST_CASE ("HardwareCcDatabase::merge: overlay ueberschreibt gleiche CC, ergaenzt neue", "[hardwarecc]")
{
    auto base = grid::HardwareCcDatabase::parse ("[Synth A]\n1 = Alter Name\n2 = Bleibt\n");
    const auto overlay = grid::HardwareCcDatabase::parse ("[Synth A]\n1 = Neuer Name\n3 = Neu\n");

    const auto merged = grid::HardwareCcDatabase::merge (std::move (base), overlay);

    REQUIRE (merged.size() == 1);
    REQUIRE (merged[0].params.size() == 3);
    CHECK (merged[0].params[0].name == "Neuer Name");   // ueberschrieben
    CHECK (merged[0].params[1].name == "Bleibt");        // unangetastet
    CHECK (merged[0].params[2].name == "Neu");           // ergaenzt
}

TEST_CASE ("HardwareCcDatabase: Faktor-Daten (BinaryData) laden ohne User-Datei", "[hardwarecc]")
{
    grid::HardwareCcDatabase db;
    db.load();

    // Faktor-Bestueckung (Assets/HardwareDevices.txt): mindestens die
    // fuenf Test-Geraete aus der ersten Runde (ein Hersteller je Eintrag).
    REQUIRE (db.devices().size() >= 5);

    const auto* digitakt = db.findDevice ("Elektron Digitakt");
    REQUIRE (digitakt != nullptr);

    const auto cutoffIt = std::find_if (digitakt->params.begin(), digitakt->params.end(),
        [] (const grid::HardwareCcParam& p) { return p.name == "Filter Cutoff"; });
    REQUIRE (cutoffIt != digitakt->params.end());
    CHECK (cutoffIt->cc == 74);

    CHECK (db.findDevice ("Unbekanntes Geraet") == nullptr);
}

TEST_CASE ("HardwareCcDatabase: User-Datei ergaenzt die Faktor-Daten", "[hardwarecc]")
{
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("ConduitHardwareCcDatabaseTests")
                          .getChildFile (juce::Uuid().toString() + ".txt");
    file.getParentDirectory().createDirectory();
    file.replaceWithText ("[Mein Synth]\n20 = Eigener Parameter\n");

    grid::HardwareCcDatabase db;
    db.load (file);

    REQUIRE (db.devices().size() >= 6);   // 5 Faktor-Geraete + 1 eigenes
    const auto* mine = db.findDevice ("Mein Synth");
    REQUIRE (mine != nullptr);
    REQUIRE (mine->params.size() == 1);
    CHECK (mine->params[0].cc == 20);

    // Faktor-Geraet bleibt unangetastet.
    CHECK (db.findDevice ("Elektron Digitakt") != nullptr);

    file.getParentDirectory().deleteRecursively();
}
