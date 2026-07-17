#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "Core/HardwarePresetLibrary.h"
#include "Core/HardwarePresetScanner.h"
#include "Core/Sysex/DsiSysex.h"

using conduit::HardwarePresetLibrary;
using conduit::HardwarePresetScanner;
using conduit::MidiPortHub;
using conduit::MidiRigSettings;
using conduit::RigDeviceKind;
namespace dsi = conduit::midirig::dsi;

namespace
{

struct TempFolder
{
    TempFolder()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitPresetScannerTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempFolder() { folder.deleteRecursively(); }

    juce::File folder;
};

/** Wire-Program-Dump (F0..F7) mit gesetztem Namen bauen. */
[[nodiscard]] juce::MidiMessage makeDumpReply (juce::uint8 deviceId, int bank, int program,
                                               const juce::String& name)
{
    std::vector<juce::uint8> plain (dsi::kProgramDataBytes, 0);
    for (int i = 0; i < dsi::kNameLength; ++i)
        plain[(size_t) (dsi::kNameOffset + i)] =
            i < name.length() ? (juce::uint8) name[i] : (juce::uint8) ' ';

    std::vector<juce::uint8> wire { 0xf0, 0x01, deviceId, 0x02,
                                    (juce::uint8) bank, (juce::uint8) program };
    const auto packed = dsi::packMsBit (plain.data(), (int) plain.size());
    wire.insert (wire.end(), packed.begin(), packed.end());
    wire.push_back (0xf7);
    return juce::MidiMessage (wire.data(), (int) wire.size());
}

[[nodiscard]] juce::MidiMessage makeInquiryReply (juce::uint8 familyLsb)
{
    const juce::uint8 reply[] = { 0xf0, 0x7e, 0x00, 0x06, 0x02, 0x01,
                                  familyLsb, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0xf7 };
    return juce::MidiMessage (reply, (int) sizeof (reply));
}

//==============================================================================
/** Kompletter Fahrstand: Hub mit Fake-Ports (Muster MidiPortHubTests),
    Library im Temp-Ordner, Scanner mit Fake-Clock. */
struct ScannerRig
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempFolder settingsFolder, presetFolder;
    MidiRigSettings settings;
    juce::Uuid device;

    juce::MidiInputCallback* inputCallback = nullptr;
    std::vector<juce::MidiMessage> sent;

    struct FakeIn final : conduit::midirig::InputPortHandle {};
    struct FakeOut final : conduit::midirig::OutputPortHandle
    {
        explicit FakeOut (ScannerRig& rigToUse) : rig (rigToUse) {}
        void send (const juce::MidiMessage& m) override { rig.sent.push_back (m); }
        ScannerRig& rig;
    };

    MidiPortHub hub;
    HardwarePresetLibrary library { presetFolder.folder };
    HardwarePresetScanner scanner { hub, library };

    double fakeNowMs = 50000.0;
    std::vector<std::pair<int, int>> progress;
    std::vector<bool> finished;

    ScannerRig()
        : settings ([this]
                    {
                        juce::PropertiesFile::Options o;
                        o.applicationName = "PresetScannerTests";
                        o.filenameSuffix = ".settings";
                        o.folderName = settingsFolder.folder.getFullPathName();
                        return o;
                    }()),
          hub (settings,
               [] { return juce::Array<juce::MidiDeviceInfo> { juce::MidiDeviceInfo ("Mopho", "in") }; },
               [] { return juce::Array<juce::MidiDeviceInfo> { juce::MidiDeviceInfo ("Mopho", "out") }; },
               [this] (const juce::String&, juce::MidiInputCallback& callback)
                   -> std::unique_ptr<conduit::midirig::InputPortHandle>
               {
                   inputCallback = &callback;
                   return std::make_unique<FakeIn>();
               },
               [this] (const juce::String&)
                   -> std::unique_ptr<conduit::midirig::OutputPortHandle>
               { return std::make_unique<FakeOut> (*this); })
    {
        device = settings.addDevice ("Mopho", RigDeviceKind::soundGenerator);
        settings.setMidiInName (device, "Mopho");
        settings.setMidiOutName (device, "Mopho");
        hub.syncFromRegistry();
        REQUIRE (hub.isInputConnected (device));
        REQUIRE (inputCallback != nullptr);

        scanner.nowMs = [this] { return fakeNowMs; };
        scanner.onProgress = [this] (int done, int total) { progress.emplace_back (done, total); };
        scanner.onFinished = [this] (bool ok) { finished.push_back (ok); };
    }

    void feed (const juce::MidiMessage& message)
    {
        inputCallback->handleIncomingMidiMessage (nullptr, message);
        hub.drainNow();
    }

    /** Letzter gesendeter Dump-Request als (deviceId, bank, program). */
    [[nodiscard]] std::optional<std::tuple<int, int, int>> lastDumpRequest() const
    {
        for (auto it = sent.rbegin(); it != sent.rend(); ++it)
        {
            const auto* raw = it->getRawData();
            if (it->getRawDataSize() == 7 && raw[1] == 0x01 && raw[3] == 0x05)
                return std::make_tuple ((int) raw[2], (int) raw[4], (int) raw[5]);
        }
        return std::nullopt;
    }

    /** Beantwortet den jeweils letzten Request, bis der Scan fertig ist. */
    void answerAllDumps (juce::uint8 deviceId)
    {
        while (scanner.isScanning())
        {
            const auto request = lastDumpRequest();
            REQUIRE (request.has_value());
            const auto [dev, bank, program] = *request;
            REQUIRE (dev == (int) deviceId);
            feed (makeDumpReply (deviceId, bank, program,
                                 "B" + juce::String (bank) + "P" + juce::String (program)));
        }
    }
};

} // namespace

//==============================================================================
TEST_CASE ("PresetScanner: Inquiry-Antwort setzt die Device-ID, Voll-Scan fuellt die Library", "[midirig][sysex]")
{
    ScannerRig rig;
    REQUIRE (rig.scanner.start (rig.device));
    CHECK_FALSE (rig.scanner.start (rig.device));   // laeuft schon

    // Erst die Inquiry, noch kein Dump-Request.
    REQUIRE (rig.sent.size() == 1);
    CHECK (rig.sent[0].getRawData()[1] == 0x7e);

    // Geraet meldet sich als Mopho Keyboard (0x27) -> Requests nutzen 0x27.
    rig.feed (makeInquiryReply (dsi::kDeviceIdMophoKeyboard));
    auto request = rig.lastDumpRequest();
    REQUIRE (request.has_value());
    CHECK (std::get<0> (*request) == (int) dsi::kDeviceIdMophoKeyboard);
    CHECK (std::get<1> (*request) == 0);
    CHECK (std::get<2> (*request) == 0);

    rig.answerAllDumps (dsi::kDeviceIdMophoKeyboard);

    REQUIRE (rig.finished == std::vector<bool> { true });
    REQUIRE (rig.progress.size() == (size_t) rig.scanner.totalPrograms());
    CHECK (rig.progress.back().first == rig.scanner.totalPrograms());

    CHECK (rig.library.hasPresets (rig.device));
    CHECK (rig.library.bankCount (rig.device) == dsi::kBankCount);
    CHECK (rig.library.presetName (rig.device, 0, 0) == "B0P0");
    CHECK (rig.library.presetName (rig.device, 2, 127) == "B2P127");
    CHECK (rig.library.deviceIdByte (rig.device) == dsi::kDeviceIdMophoKeyboard);
}

TEST_CASE ("PresetScanner: Inquiry-Timeout faellt auf die Fallback-Device-ID zurueck", "[midirig][sysex]")
{
    ScannerRig rig;
    REQUIRE (rig.scanner.start (rig.device));

    rig.fakeNowMs += HardwarePresetScanner::kInquiryTimeoutMs + 1.0;
    rig.hub.drainNow();

    const auto request = rig.lastDumpRequest();
    REQUIRE (request.has_value());
    CHECK (std::get<0> (*request) == (int) dsi::kDeviceIdMophoDesktop);   // 0x25
}

TEST_CASE ("PresetScanner: Dump-Timeout -> 2 Retries -> Platzhalter und weiter", "[midirig][sysex]")
{
    ScannerRig rig;
    REQUIRE (rig.scanner.start (rig.device));
    rig.feed (makeInquiryReply (dsi::kDeviceIdMophoDesktop));

    const auto requestsForCurrent = [&rig]
    {
        int count = 0;
        for (const auto& m : rig.sent)
            if (m.getRawDataSize() == 7 && m.getRawData()[3] == 0x05
                && m.getRawData()[4] == 0 && m.getRawData()[5] == 0)
                ++count;
        return count;
    };
    CHECK (requestsForCurrent() == 1);

    // 1. + 2. Timeout: Re-Requests derselben Position.
    for (int retry = 2; retry <= 3; ++retry)
    {
        rig.fakeNowMs += HardwarePresetScanner::kDumpTimeoutMs + 1.0;
        rig.hub.drainNow();
        CHECK (requestsForCurrent() == retry);
    }

    // 3. Timeout: "?"-Name, naechste Position angefragt.
    rig.fakeNowMs += HardwarePresetScanner::kDumpTimeoutMs + 1.0;
    rig.hub.drainNow();
    CHECK (requestsForCurrent() == 3);

    const auto request = rig.lastDumpRequest();
    REQUIRE (request.has_value());
    CHECK (std::get<2> (*request) == 1);   // Programm 1
    REQUIRE (! rig.progress.empty());
    CHECK (rig.progress.back().first == 1);

    // Verspaeteter Dump der ALTEN Position wird ignoriert (kein Doppel-Advance).
    rig.feed (makeDumpReply (dsi::kDeviceIdMophoDesktop, 0, 0, "Late"));
    CHECK (rig.progress.back().first == 1);

    // Der Rest laeuft normal durch; Position 0 traegt den Platzhalter.
    rig.answerAllDumps (dsi::kDeviceIdMophoDesktop);
    CHECK (rig.library.presetName (rig.device, 0, 0) == "?");
    CHECK (rig.library.presetName (rig.device, 0, 1) == "B0P1");
}

TEST_CASE ("PresetScanner: cancel disarmt ohne Library-Schreiben", "[midirig][sysex]")
{
    ScannerRig rig;
    REQUIRE (rig.scanner.start (rig.device));
    rig.feed (makeInquiryReply (dsi::kDeviceIdMophoDesktop));
    rig.feed (makeDumpReply (dsi::kDeviceIdMophoDesktop, 0, 0, "First"));

    rig.scanner.cancel();
    CHECK_FALSE (rig.scanner.isScanning());
    CHECK (rig.finished == std::vector<bool> { false });
    CHECK_FALSE (rig.library.hasPresets (rig.device));

    // Disarmed: spaetere Dumps erreichen niemanden mehr.
    const auto sentBefore = rig.sent.size();
    rig.feed (makeDumpReply (dsi::kDeviceIdMophoDesktop, 0, 1, "Ghost"));
    CHECK (rig.sent.size() == sentBefore);

    // Neustart moeglich.
    CHECK (rig.scanner.start (rig.device));
}

//==============================================================================
TEST_CASE ("PresetLibrary: Persistenz-Roundtrip + korrupte Datei", "[midirig][sysex]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempFolder temp;
    const juce::Uuid device;

    {
        HardwarePresetLibrary library { temp.folder };
        HardwarePresetLibrary::DevicePresets presets;
        presets.deviceIdByte = dsi::kDeviceIdMophoDesktop;
        presets.programsPerBank = 4;
        presets.names = { "A", "B", "C", "D", "E", "F", "G", "H" };
        library.setPresets (device, std::move (presets));
    }

    // Neue Instanz laedt aus dem Ordner.
    HardwarePresetLibrary reloaded { temp.folder };
    REQUIRE (reloaded.hasPresets (device));
    CHECK (reloaded.bankCount (device) == 2);
    CHECK (reloaded.programsPerBank (device) == 4);
    CHECK (reloaded.presetName (device, 0, 0) == "A");
    CHECK (reloaded.presetName (device, 1, 3) == "H");
    CHECK (reloaded.presetName (device, 2, 0) == juce::String());   // ausserhalb
    CHECK (reloaded.deviceIdByte (device) == dsi::kDeviceIdMophoDesktop);

    // clearPresets loescht Datei + Cache.
    reloaded.clearPresets (device);
    CHECK_FALSE (reloaded.hasPresets (device));
    HardwarePresetLibrary again { temp.folder };
    CHECK_FALSE (again.hasPresets (device));

    // Korrupte XML -> leerer Cache statt Crash.
    temp.folder.getChildFile (juce::Uuid().toString() + ".xml")
        .replaceWithText ("<<<kein xml>>>");
    HardwarePresetLibrary corrupt { temp.folder };
    CHECK_FALSE (corrupt.hasPresets (device));
}
