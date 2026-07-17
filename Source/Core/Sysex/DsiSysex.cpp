#include "DsiSysex.h"

namespace conduit::midirig::dsi
{

namespace
{
    constexpr juce::uint8 kSysExStart = 0xf0;
    constexpr juce::uint8 kSysExEnd = 0xf7;
    constexpr juce::uint8 kUniversalNonRealtime = 0x7e;
    constexpr juce::uint8 kGeneralInformation = 0x06;
    constexpr juce::uint8 kInquiryRequest = 0x01;
    constexpr juce::uint8 kInquiryReply = 0x02;
    constexpr juce::uint8 kCmdProgramData = 0x02;
    constexpr juce::uint8 kCmdRequestProgramDump = 0x05;
}

//==============================================================================
std::vector<juce::uint8> unpackMsBit (const juce::uint8* packed, int packedSize)
{
    std::vector<juce::uint8> result;
    if (packed == nullptr || packedSize <= 0)
        return result;

    result.reserve ((static_cast<size_t> (packedSize) * 7u) / 8u);

    for (int groupStart = 0; groupStart < packedSize; groupStart += 8)
    {
        const auto msBits = packed[groupStart];
        const auto groupBytes = juce::jmin (7, packedSize - groupStart - 1);

        for (int i = 0; i < groupBytes; ++i)
        {
            const auto ms = static_cast<juce::uint8> (((msBits >> i) & 0x01u) << 7);
            result.push_back (static_cast<juce::uint8> (packed[groupStart + 1 + i] | ms));
        }
    }

    return result;
}

std::vector<juce::uint8> packMsBit (const juce::uint8* plain, int plainSize)
{
    std::vector<juce::uint8> result;
    if (plain == nullptr || plainSize <= 0)
        return result;

    for (int groupStart = 0; groupStart < plainSize; groupStart += 7)
    {
        const auto groupBytes = juce::jmin (7, plainSize - groupStart);

        juce::uint8 msBits = 0;
        for (int i = 0; i < groupBytes; ++i)
            if ((plain[groupStart + i] & 0x80u) != 0)
                msBits = static_cast<juce::uint8> (msBits | (1u << i));

        result.push_back (msBits);
        for (int i = 0; i < groupBytes; ++i)
            result.push_back (static_cast<juce::uint8> (plain[groupStart + i] & 0x7fu));
    }

    return result;
}

//==============================================================================
juce::MidiMessage makeProgramDumpRequest (juce::uint8 deviceId, int bank, int program)
{
    const juce::uint8 payload[] = { kDsiManufacturerId, deviceId, kCmdRequestProgramDump,
                                    static_cast<juce::uint8> (juce::jlimit (0, kBankCount - 1, bank)),
                                    static_cast<juce::uint8> (juce::jlimit (0, kProgramsPerBank - 1,
                                                                            program)) };
    return juce::MidiMessage::createSysExMessage (payload, (int) sizeof (payload));
}

juce::MidiMessage makeDeviceInquiry()
{
    const juce::uint8 payload[] = { kUniversalNonRealtime, 0x7f,
                                    kGeneralInformation, kInquiryRequest };
    return juce::MidiMessage::createSysExMessage (payload, (int) sizeof (payload));
}

//==============================================================================
std::optional<ProgramDump> parseProgramDump (const juce::uint8* data, int size)
{
    // F0 01 <dev> 02 <bank> <prog> <packed...> F7 — Minimum: Header + EOX.
    constexpr int kHeaderBytes = 6;
    if (data == nullptr || size < kHeaderBytes + 1
        || data[0] != kSysExStart || data[1] != kDsiManufacturerId
        || data[3] != kCmdProgramData || data[size - 1] != kSysExEnd)
        return std::nullopt;

    ProgramDump dump;
    dump.deviceId = data[2];
    dump.bank = data[4];
    dump.program = data[5];
    dump.data = unpackMsBit (data + kHeaderBytes, size - kHeaderBytes - 1);

    if ((int) dump.data.size() < kProgramDataBytes)
        return std::nullopt;   // verstuemmelter Dump

    return dump;
}

juce::String extractName (const std::vector<juce::uint8>& unpacked)
{
    if ((int) unpacked.size() < kNameOffset + kNameLength)
        return {};

    juce::String name;
    for (int i = 0; i < kNameLength; ++i)
    {
        const auto byte = unpacked[(size_t) (kNameOffset + i)];
        const auto printable = byte >= 0x20 && byte < 0x7f;
        name << (juce::juce_wchar) (printable ? byte : (juce::uint8) '?');
    }

    return name.trimEnd();
}

//==============================================================================
std::optional<InquiryReply> parseDeviceInquiryReply (const juce::uint8* data, int size)
{
    // F0 7E <ch> 06 02 <manu> <family lsb/msb> <member lsb/msb> ... F7
    // (Ein-Byte-Manufacturer; Drei-Byte-IDs beginnen mit 0x00 — hier nicht
    // relevant, DSI ist 0x01. Fremde Aufbauten -> nullopt.)
    constexpr int kMinBytes = 11;
    if (data == nullptr || size < kMinBytes
        || data[0] != kSysExStart || data[1] != kUniversalNonRealtime
        || data[3] != kGeneralInformation || data[4] != kInquiryReply
        || data[5] == 0x00 || data[size - 1] != kSysExEnd)
        return std::nullopt;

    InquiryReply reply;
    reply.manufacturerId = data[5];
    reply.familyLsb = data[6];
    reply.familyMsb = data[7];
    reply.memberLsb = data[8];
    reply.memberMsb = data[9];
    return reply;
}

std::optional<juce::uint8> deviceIdFromInquiry (const InquiryReply& reply)
{
    if (reply.manufacturerId != kDsiManufacturerId)
        return std::nullopt;

    return reply.familyLsb;
}

} // namespace conduit::midirig::dsi
