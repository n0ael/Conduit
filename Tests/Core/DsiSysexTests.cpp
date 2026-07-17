#include <algorithm>
#include <cstring>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "Core/Sysex/DsiSysex.h"

namespace dsi = conduit::midirig::dsi;

namespace
{

/** Kompletter Wire-Dump (F0...F7) aus einem 256-Byte-Programm bauen —
    die Golden-Bytes-Gegenprobe fuer parseProgramDump. */
[[nodiscard]] std::vector<juce::uint8> makeWireDump (juce::uint8 deviceId, int bank, int program,
                                                     const std::vector<juce::uint8>& plain)
{
    std::vector<juce::uint8> wire { 0xf0, 0x01, deviceId, 0x02,
                                    (juce::uint8) bank, (juce::uint8) program };
    const auto packed = dsi::packMsBit (plain.data(), (int) plain.size());
    wire.insert (wire.end(), packed.begin(), packed.end());
    wire.push_back (0xf7);
    return wire;
}

[[nodiscard]] std::vector<juce::uint8> makeProgram (const char* name)
{
    std::vector<juce::uint8> plain (dsi::kProgramDataBytes, 0);
    for (size_t i = 0; i < plain.size(); ++i)
        plain[i] = (juce::uint8) ((i * 7 + 13) & 0xff);   // Muster mit MS-Bits

    for (int i = 0; i < dsi::kNameLength; ++i)
        plain[(size_t) (dsi::kNameOffset + i)] =
            i < (int) std::strlen (name) ? (juce::uint8) name[i] : (juce::uint8) ' ';

    return plain;
}

} // namespace

//==============================================================================
TEST_CASE ("DsiSysex: Packed-MS-Bit Roundtrip 256 Bytes -> 293 Wire-Bytes", "[midirig][sysex]")
{
    const auto plain = makeProgram ("Roundtrip");
    const auto packed = dsi::packMsBit (plain.data(), (int) plain.size());

    // 256 / 7 = 36 volle Gruppen (8 Bytes) + Rest 4 -> 36*8 + 1 + 4 = 293.
    REQUIRE ((int) packed.size() == 293);

    // Alle Wire-Bytes 7-bit-clean (MIDI-Datenbytes).
    for (const auto byte : packed)
        REQUIRE ((byte & 0x80) == 0);

    const auto unpacked = dsi::unpackMsBit (packed.data(), (int) packed.size());
    REQUIRE (unpacked == plain);
}

TEST_CASE ("DsiSysex: parseProgramDump liefert Bank/Programm/Daten exakt", "[midirig][sysex]")
{
    const auto plain = makeProgram ("Fat Bass 01");
    const auto wire = makeWireDump (dsi::kDeviceIdMophoDesktop, 2, 87, plain);

    const auto dump = dsi::parseProgramDump (wire.data(), (int) wire.size());
    REQUIRE (dump.has_value());
    CHECK (dump->deviceId == dsi::kDeviceIdMophoDesktop);
    CHECK (dump->bank == 2);
    CHECK (dump->program == 87);
    REQUIRE ((int) dump->data.size() >= dsi::kProgramDataBytes);
    CHECK (std::equal (plain.begin(), plain.end(), dump->data.begin()));

    CHECK (dsi::extractName (dump->data) == "Fat Bass 01");
}

TEST_CASE ("DsiSysex: parseProgramDump weist Fremdes und Verstuemmeltes ab", "[midirig][sysex]")
{
    const auto plain = makeProgram ("X");
    auto wire = makeWireDump (dsi::kDeviceIdMophoDesktop, 0, 0, plain);

    SECTION ("fremder Hersteller")
    {
        wire[1] = 0x43;   // Yamaha
        CHECK_FALSE (dsi::parseProgramDump (wire.data(), (int) wire.size()).has_value());
    }

    SECTION ("falsches Kommando (Request statt Data)")
    {
        wire[3] = 0x05;
        CHECK_FALSE (dsi::parseProgramDump (wire.data(), (int) wire.size()).has_value());
    }

    SECTION ("abgeschnittener Dump -> zu wenig Daten")
    {
        std::vector<juce::uint8> cut (wire.begin(), wire.begin() + 100);
        cut.push_back (0xf7);
        CHECK_FALSE (dsi::parseProgramDump (cut.data(), (int) cut.size()).has_value());
    }

    SECTION ("leer / zu kurz")
    {
        CHECK_FALSE (dsi::parseProgramDump (nullptr, 0).has_value());
        const juce::uint8 tiny[] = { 0xf0, 0x01, 0xf7 };
        CHECK_FALSE (dsi::parseProgramDump (tiny, 3).has_value());
    }
}

TEST_CASE ("DsiSysex: extractName trimmt Spaces und ersetzt Nicht-Druckbares", "[midirig][sysex]")
{
    auto plain = makeProgram ("Pad");   // "Pad" + 13 Spaces
    CHECK (dsi::extractName (plain) == "Pad");

    plain[(size_t) dsi::kNameOffset + 1] = 0x07;   // Steuerzeichen -> '?'
    CHECK (dsi::extractName (plain) == "P?d");

    CHECK (dsi::extractName (std::vector<juce::uint8> (10, 0)) == juce::String());
}

TEST_CASE ("DsiSysex: Request-Builder erzeugen korrekte Wire-Bytes", "[midirig][sysex]")
{
    const auto request = dsi::makeProgramDumpRequest (dsi::kDeviceIdMophoDesktop, 1, 64);
    const auto* raw = request.getRawData();
    REQUIRE (request.getRawDataSize() == 7);   // F0 + 5 Payload + F7
    CHECK (raw[0] == 0xf0);
    CHECK (raw[1] == 0x01);
    CHECK (raw[2] == 0x25);
    CHECK (raw[3] == 0x05);
    CHECK (raw[4] == 1);
    CHECK (raw[5] == 64);
    CHECK (raw[6] == 0xf7);

    // Grenzen werden geklemmt (Bank 0-2, Programm 0-127).
    const auto clamped = dsi::makeProgramDumpRequest (dsi::kDeviceIdMophoDesktop, 9, 500);
    CHECK (clamped.getRawData()[4] == 2);
    CHECK (clamped.getRawData()[5] == 127);

    const auto inquiry = dsi::makeDeviceInquiry();
    const auto* inquiryRaw = inquiry.getRawData();
    REQUIRE (inquiry.getRawDataSize() == 6);
    CHECK (inquiryRaw[1] == 0x7e);
    CHECK (inquiryRaw[2] == 0x7f);
    CHECK (inquiryRaw[3] == 0x06);
    CHECK (inquiryRaw[4] == 0x01);
}

TEST_CASE ("DsiSysex: Inquiry-Reply-Parser", "[midirig][sysex]")
{
    // F0 7E <ch> 06 02 <manu> <family l/m> <member l/m> <version...> F7
    const juce::uint8 reply[] = { 0xf0, 0x7e, 0x00, 0x06, 0x02, 0x01,
                                  0x25, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0xf7 };

    const auto parsed = dsi::parseDeviceInquiryReply (reply, (int) sizeof (reply));
    REQUIRE (parsed.has_value());
    CHECK (parsed->manufacturerId == 0x01);
    CHECK (parsed->familyLsb == 0x25);

    const auto deviceId = dsi::deviceIdFromInquiry (*parsed);
    REQUIRE (deviceId.has_value());
    CHECK (*deviceId == dsi::kDeviceIdMophoDesktop);

    SECTION ("fremder Hersteller -> keine Device-ID")
    {
        auto foreign = *parsed;
        foreign.manufacturerId = 0x43;
        CHECK_FALSE (dsi::deviceIdFromInquiry (foreign).has_value());
    }

    SECTION ("kein Inquiry-Reply / zu kurz")
    {
        auto broken = std::vector<juce::uint8> (reply, reply + sizeof (reply));
        broken[4] = 0x01;   // Request statt Reply
        CHECK_FALSE (dsi::parseDeviceInquiryReply (broken.data(), (int) broken.size()).has_value());

        CHECK_FALSE (dsi::parseDeviceInquiryReply (reply, 5).has_value());
    }
}
