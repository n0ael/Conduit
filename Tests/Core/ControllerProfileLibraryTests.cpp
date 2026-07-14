#include <catch2/catch_test_macros.hpp>

#include "Core/ControllerProfileLibrary.h"

using conduit::ControllerProfileLibrary;

namespace
{

struct TempFolder
{
    TempFolder()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitControllerProfileLibraryTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempFolder() { folder.deleteRecursively(); }

    juce::File folder;
};

} // namespace

//==============================================================================
TEST_CASE ("ControllerProfileLibrary: Factory-Profil Xone:K1 aus BinaryData", "[midirig]")
{
    ControllerProfileLibrary library;   // kein User-Ordner

    REQUIRE (library.profiles().size() >= 1);

    const auto* k1 = library.find ("Xone:K1");
    REQUIRE (k1 != nullptr);
    REQUIRE_FALSE (k1->controls.empty());

    // Report: eine Factory-Quelle, keine User-Quellen.
    REQUIRE (library.report().size() >= 1);
    for (const auto& source : library.report())
        REQUIRE_FALSE (source.fromUserFolder);
}

TEST_CASE ("ControllerProfileLibrary: User-Ordner-Scan FLACH (kein Hersteller-Unterordner)", "[midirig]")
{
    TempFolder temp;
    temp.folder.getChildFile ("MyController.csv").replaceWithText (
        "device,id,send_number\n"
        "MyController,knob1,10\n");

    ControllerProfileLibrary library { temp.folder };

    const auto* mine = library.find ("MyController");
    REQUIRE (mine != nullptr);
    REQUIRE (mine->controls.size() == 1);
    CHECK (mine->controls[0].id == "knob1");
}

TEST_CASE ("ControllerProfileLibrary: fehlendes device-Feld -- Dateiname als Fallback", "[midirig]")
{
    TempFolder temp;
    temp.folder.getChildFile ("NoDeviceColumn.csv").replaceWithText (
        "id,send_number\n"
        "knob1,10\n");

    ControllerProfileLibrary library { temp.folder };

    const auto* fallback = library.find ("NoDeviceColumn");
    REQUIRE (fallback != nullptr);
    REQUIRE (fallback->controls.size() == 1);
}

TEST_CASE ("ControllerProfileLibrary: User-Profil ersetzt Factory komplett bei gleichem device", "[midirig]")
{
    TempFolder temp;
    temp.folder.getChildFile ("XoneK1Override.csv").replaceWithText (
        "device,id,send_number\n"
        "Xone:K1,onlyThis,99\n");

    ControllerProfileLibrary library { temp.folder };

    const auto* k1 = library.find ("Xone:K1");
    REQUIRE (k1 != nullptr);
    REQUIRE (k1->controls.size() == 1);
    CHECK (k1->controls[0].id == "onlyThis");   // Factory-Controls komplett ersetzt, nicht gemergt

    bool sawUserSource = false;
    for (const auto& source : library.report())
        if (source.fromUserFolder)
            sawUserSource = true;
    CHECK (sawUserSource);
}

TEST_CASE ("ControllerProfileLibrary: leere/kaputte User-CSV -- Warning im Report, kein Crash", "[midirig]")
{
    TempFolder temp;
    temp.folder.getChildFile ("Broken.csv").replaceWithText ("nur ein paar Worte ohne Sinn\n");

    ControllerProfileLibrary library { temp.folder };

    bool sawWarning = false;
    for (const auto& source : library.report())
        if (source.fromUserFolder && ! source.parse.warnings.isEmpty())
            sawWarning = true;
    CHECK (sawWarning);
}
