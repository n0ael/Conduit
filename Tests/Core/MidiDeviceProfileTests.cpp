#include <catch2/catch_test_macros.hpp>

#include "Core/MidiDeviceProfile.h"

using conduit::midirig::ParseReport;
using conduit::midirig::parseMidiGuideCsv;

//==============================================================================
TEST_CASE ("parseMidiGuideCsv: CC-, NRPN- und Misch-Parameter mit Ranges", "[midirig]")
{
    const juce::String csv =
        "manufacturer,device,section,parameter_name,cc_msb,cc_min_value,cc_max_value,"
        "nrpn_msb,nrpn_lsb,nrpn_min_value,nrpn_max_value\n"
        "Elektron,Analog Heat,,Drive,,,,0,12,0,16383\n"
        "Arturia,MicroFreak,Oscillator,Type,9,0,127,,,,\n"
        "Elektron,Analog Heat,Filter,Frequency,74,0,127,0,70,0,16383\n";

    ParseReport report;
    const auto profiles = parseMidiGuideCsv (csv, &report);

    REQUIRE (profiles.size() == 2);
    REQUIRE (report.accepted == 3);
    REQUIRE (report.skipped == 0);

    const auto& heat = profiles[0];
    REQUIRE (heat.manufacturer == "Elektron");
    REQUIRE (heat.device == "Analog Heat");
    REQUIRE (heat.displayName() == "Elektron Analog Heat");
    REQUIRE (heat.params.size() == 2);

    REQUIRE (heat.params[0].name == "Drive");
    REQUIRE (heat.params[0].cc == -1);
    REQUIRE (heat.params[0].nrpn == 12);      // 0*128 + 12
    REQUIRE (heat.params[0].minValue == 0);
    REQUIRE (heat.params[0].maxValue == 16383);

    // Misch-Param: CC UND NRPN — NRPN-Range hat Vorrang.
    REQUIRE (heat.params[1].name == "Frequency");
    REQUIRE (heat.params[1].section == "Filter");
    REQUIRE (heat.params[1].cc == 74);
    REQUIRE (heat.params[1].nrpn == 70);
    REQUIRE (heat.params[1].maxValue == 16383);

    const auto& freak = profiles[1];
    REQUIRE (freak.params.size() == 1);
    REQUIRE (freak.params[0].cc == 9);
    REQUIRE (freak.params[0].nrpn == -1);
    REQUIRE (freak.params[0].maxValue == 127);
}

TEST_CASE ("parseMidiGuideCsv: Spalten in anderer Reihenfolge + unbekannte Spalten", "[midirig]")
{
    const juce::String csv =
        "usage,parameter_name,foo_unknown,manufacturer,cc_msb,device\n"
        ",Cutoff,x,Moog,19,Minitaur\n";

    const auto profiles = parseMidiGuideCsv (csv);
    REQUIRE (profiles.size() == 1);
    REQUIRE (profiles[0].manufacturer == "Moog");
    REQUIRE (profiles[0].device == "Minitaur");
    REQUIRE (profiles[0].params[0].name == "Cutoff");
    REQUIRE (profiles[0].params[0].cc == 19);
}

TEST_CASE ("parseMidiGuideCsv: CSV-Quoting mit Kommas und escaped Quotes", "[midirig]")
{
    const juce::String csv =
        "manufacturer,device,section,parameter_name,cc_msb\n"
        "Waldorf,Blofeld,\"Osc 1, Pitch\",\"Semitone \"\"coarse\"\"\",27\n";

    const auto profiles = parseMidiGuideCsv (csv);
    REQUIRE (profiles.size() == 1);
    REQUIRE (profiles[0].params[0].section == "Osc 1, Pitch");
    REQUIRE (profiles[0].params[0].name == "Semitone \"coarse\"");
    REQUIRE (profiles[0].params[0].cc == 27);
}

TEST_CASE ("parseMidiGuideCsv: Zeilen ohne Name/Adresse werden gezaehlt und uebersprungen", "[midirig]")
{
    const juce::String csv =
        "manufacturer,device,parameter_name,cc_msb,nrpn_msb,nrpn_lsb\n"
        "A,B,,74,,\n"              // kein Name
        "A,B,NoAddress,,,\n"       // weder CC noch NRPN
        "A,B,Valid,74,,\n"
        "\n";                       // Leerzeile ignoriert

    ParseReport report;
    const auto profiles = parseMidiGuideCsv (csv, &report);

    REQUIRE (profiles.size() == 1);
    REQUIRE (profiles[0].params.size() == 1);
    REQUIRE (report.accepted == 1);
    REQUIRE (report.skipped == 2);
    REQUIRE (report.warnings.size() == 1);   // nur die Name-tragende Skip-Zeile warnt
}

TEST_CASE ("parseMidiGuideCsv: kaputte Eingaben crashen nicht", "[midirig]")
{
    ParseReport report;
    REQUIRE (parseMidiGuideCsv ("", &report).empty());
    REQUIRE (parseMidiGuideCsv ("kein,gueltiger,header\n1,2,3\n", &report).empty());
    REQUIRE (report.warnings.size() == 1);   // Header-Warnung

    // Verdrehte Range wird repariert.
    const auto profiles = parseMidiGuideCsv (
        "manufacturer,device,parameter_name,cc_msb,cc_min_value,cc_max_value\n"
        "A,B,Broken,74,100,10\n");
    REQUIRE (profiles[0].params[0].minValue == 0);
    REQUIRE (profiles[0].params[0].maxValue == 127);
}
