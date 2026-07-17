#pragma once

#include <optional>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

namespace conduit::midirig::dsi
{

//==============================================================================
/** DSI-SysEx-Dialekt (ADR 007, M9a) — pure, hardwarefreie Funktionen fuer
    das Dave-Smith-Protokoll (Mopho; Prophet '08/Rev2/Tetra teilen Aufbau
    und Packed-Format, nur die Device-ID-Bytes unterscheiden sich).

    Byte-Referenz (Mopho Operation Manual, SysEx-Kapitel):
      Request Program Dump:  F0 01 <dev> 05 <bank 0-2> <prog 0-127> F7
      Program Data Dump:     F0 01 <dev> 02 <bank> <prog> <293 Bytes packed> F7
      Universal Inquiry:     F0 7E 7F 06 01 F7
      Inquiry Reply:         F0 7E <ch> 06 02 01 <family...> ... F7

    Alle Parser arbeiten auf der KOMPLETTEN Wire-Nachricht (inkl. F0/F7),
    wie sie der SysEx-Transport des MidiPortHub ausliefert. Kein Zustand,
    kein Geraetezugriff — Catch2-golden-bytes-testbar. */

//==============================================================================
// Device-IDs (DSI-Familie; Fallback wenn die Inquiry unbeantwortet bleibt)
inline constexpr juce::uint8 kDsiManufacturerId = 0x01;
inline constexpr juce::uint8 kDeviceIdMophoDesktop = 0x25;
inline constexpr juce::uint8 kDeviceIdMophoKeyboard = 0x27;

// Mopho-Programm-Layout
inline constexpr int kProgramDataBytes = 256;   // entpackte Programmgroesse
inline constexpr int kNameOffset = 184;         // Name im entpackten Dump ...
inline constexpr int kNameLength = 16;          // ... 16 ASCII-Zeichen
inline constexpr int kBankCount = 3;
inline constexpr int kProgramsPerBank = 128;

//==============================================================================
/** Packed-MS-Bit dekodieren (DSI-Standard): Gruppen aus 1 MS-Bit-Byte +
    bis zu 7 Datenbytes; Bit i des MS-Bytes ist Bit 7 des i-ten Datenbytes.
    256 Datenbytes reisen so als 293 Wire-Bytes. */
[[nodiscard]] std::vector<juce::uint8> unpackMsBit (const juce::uint8* packed, int packedSize);

/** Gegenrichtung (nur fuer Test-Golden-Bytes gebraucht). */
[[nodiscard]] std::vector<juce::uint8> packMsBit (const juce::uint8* plain, int plainSize);

//==============================================================================
/** Request Program Dump an ein DSI-Geraet. */
[[nodiscard]] juce::MidiMessage makeProgramDumpRequest (juce::uint8 deviceId,
                                                        int bank, int program);

/** Universal Device Inquiry (herstellerneutral, Broadcast-Kanal 0x7F). */
[[nodiscard]] juce::MidiMessage makeDeviceInquiry();

//==============================================================================
struct ProgramDump
{
    juce::uint8 deviceId = 0;
    int bank = 0;
    int program = 0;
    std::vector<juce::uint8> data;   // entpackt, >= kProgramDataBytes
};

/** Program-Data-Dump parsen; nullopt wenn keine DSI-Programmnachricht oder
    zu wenig Daten. deviceId wird NICHT gefiltert (der Scanner entscheidet). */
[[nodiscard]] std::optional<ProgramDump> parseProgramDump (const juce::uint8* data, int size);

/** Programmname aus dem entpackten Dump: 16 Zeichen ab kNameOffset,
    Nicht-Druckbares wird '?', Trailing-Spaces getrimmt. */
[[nodiscard]] juce::String extractName (const std::vector<juce::uint8>& unpacked);

//==============================================================================
struct InquiryReply
{
    juce::uint8 manufacturerId = 0;   // 0x01 = DSI
    juce::uint8 familyLsb = 0;        // DSI: Device-ID-Byte (0x25 Mopho Desktop)
    juce::uint8 familyMsb = 0;
    juce::uint8 memberLsb = 0;
    juce::uint8 memberMsb = 0;
};

/** Universal-Inquiry-Antwort parsen (F0 7E <ch> 06 02 <manu> <family...>).
    nullopt bei fremdem Aufbau oder zu kurzer Nachricht. */
[[nodiscard]] std::optional<InquiryReply> parseDeviceInquiryReply (const juce::uint8* data,
                                                                   int size);

/** Device-ID-Byte fuer Dump-Requests aus einer Inquiry-Antwort — nur fuer
    DSI-Geraete (Manufacturer 0x01), sonst nullopt. */
[[nodiscard]] std::optional<juce::uint8> deviceIdFromInquiry (const InquiryReply& reply);

} // namespace conduit::midirig::dsi
