#include <catch2/catch_test_macros.hpp>

#include "Core/ControllerProfileLibrary.h"

using conduit::ControllerProfileLibrary;
namespace midirig = conduit::midirig;

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

    // M6: Spalten-Status-Controls (Encoder-Reihe-1-Pushes) mit den drei
    // Status-Farben + Gruppen-Zuordnung der Member (Fader 1 in col1).
    const auto* statusPush = k1->findBySendAddress (midirig::AddressKind::note, 52);
    REQUIRE (statusPush != nullptr);
    CHECK (statusPush->group == "col1");
    REQUIRE (statusPush->feedback.size() == 3);
    CHECK (statusPush->feedback[0].meaning == "status_red");
    CHECK (statusPush->feedback[1].meaning == "status_amber");
    CHECK (statusPush->feedback[2].meaning == "status_green");

    const auto* fader1 = k1->findBySendAddress (midirig::AddressKind::cc, 16);
    REQUIRE (fader1 != nullptr);
    CHECK (fader1->group == "col1");
    REQUIRE (fader1->feedback.size() == 1);
    CHECK (fader1->feedback[0].meaning == "led_pickup");
    CHECK (fader1->feedback[0].number == 36);   // Pad-Reihe A-D (User 15.07.2026)

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
