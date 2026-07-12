#include <catch2/catch_test_macros.hpp>

#include "Core/MidiProfileLibrary.h"

using conduit::MidiProfileLibrary;

namespace
{

struct TempFolder
{
    TempFolder()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitMidiProfileLibraryTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempFolder() { folder.deleteRecursively(); }

    juce::File folder;
};

} // namespace

//==============================================================================
TEST_CASE ("MidiProfileLibrary: Factory-Profile aus BinaryData (inkl. Analog Heat mit NRPN)", "[midirig]")
{
    MidiProfileLibrary library;   // kein User-Ordner

    REQUIRE (library.profiles().size() >= 6);

    const auto* heat = library.find ("Elektron", "Analog Heat");
    REQUIRE (heat != nullptr);
    REQUIRE (! heat->params.empty());

    // Analog Heat ist NRPN-only (Block-L2-Befund) — mindestens ein Param
    // trägt eine NRPN-Adresse und die 14-bit-Range.
    bool hasNrpn = false;
    for (const auto& param : heat->params)
        if (param.nrpn >= 0 && param.maxValue > 127)
            hasNrpn = true;
    REQUIRE (hasNrpn);

    // CC-Gerät als Gegenprobe.
    const auto* freak = library.find ("Arturia", "MicroFreak");
    REQUIRE (freak != nullptr);
    REQUIRE (freak->params[0].cc >= 0);

    // Report: eine Factory-Quelle pro CSV, keine User-Quellen.
    REQUIRE (library.report().size() >= 6);
    for (const auto& source : library.report())
        REQUIRE_FALSE (source.fromUserFolder);
}

TEST_CASE ("MidiProfileLibrary: User-Ordner-Scan ({Hersteller}/{Geraet}.csv, rekursiv)", "[midirig]")
{
    TempFolder temp;
    temp.folder.getChildFile ("MyBrand").createDirectory();
    temp.folder.getChildFile ("MyBrand").getChildFile ("MySynth.csv").replaceWithText (
        "manufacturer,device,parameter_name,cc_msb\n"
        "MyBrand,MySynth,Cutoff,74\n");

    MidiProfileLibrary library { temp.folder };

    const auto* synth = library.find ("MyBrand", "MySynth");
    REQUIRE (synth != nullptr);
    REQUIRE (synth->params.size() == 1);

    bool foundUserSource = false;
    for (const auto& source : library.report())
        if (source.fromUserFolder && source.devices == 1)
            foundUserSource = true;
    REQUIRE (foundUserSource);
}

TEST_CASE ("MidiProfileLibrary: User-Profil ERSETZT gleichnamiges Factory-Profil", "[midirig]")
{
    TempFolder temp;
    temp.folder.getChildFile ("Elektron").createDirectory();
    temp.folder.getChildFile ("Elektron").getChildFile ("Analog Heat.csv").replaceWithText (
        "manufacturer,device,parameter_name,nrpn_msb,nrpn_lsb,nrpn_min_value,nrpn_max_value\n"
        "Elektron,Analog Heat,Nur Meiner,1,1,0,16383\n");

    MidiProfileLibrary library { temp.folder };

    const auto* heat = library.find ("Elektron", "Analog Heat");
    REQUIRE (heat != nullptr);
    REQUIRE (heat->params.size() == 1);            // komplett ersetzt, nicht gemerged
    REQUIRE (heat->params[0].name == "Nur Meiner");
}

TEST_CASE ("MidiProfileLibrary: kaputte User-CSV landet mit Warnung im Report", "[midirig]")
{
    TempFolder temp;
    temp.folder.getChildFile ("Broken.csv").replaceWithText ("voellig,kaputt\n1,2\n");

    MidiProfileLibrary library { temp.folder };

    bool foundWarning = false;
    for (const auto& source : library.report())
        if (source.fromUserFolder && ! source.parse.warnings.isEmpty())
            foundWarning = true;
    REQUIRE (foundWarning);
}
