#pragma once

#include <cstring>
#include <type_traits>

#include <juce_core/juce_core.h>

namespace conduit::midi
{

//==============================================================================
/** SysEx-Transport-Chunk (ADR 007, M9a): traegt ein Stueck einer kompletten
    SysEx-Nachricht (inkl. F0/F7) durch die SpscQueue vom MIDI-System-Thread
    zum Message Thread. Variable Laenge via fixe POD-Chunks — die Queue-
    Invariante (trivially copyable, CLAUDE.md 3.1) bleibt unangetastet.

    Der Producer pusht eine Nachricht NUR komplett (Platz vorab pruefen,
    nie halbe Dumps); der Reassembler auf dem Message Thread setzt Chunks
    in FIFO-Reihenfolge zusammen (offset 0 beginnt eine neue Nachricht). */
struct SysExChunk
{
    static constexpr int kPayloadBytes = 56;

    juce::uint16 totalSize = 0;   // Gesamtgroesse der Nachricht (inkl. F0/F7)
    juce::uint16 offset = 0;      // Byte-Offset dieses Chunks in der Nachricht
    juce::uint8 size = 0;         // belegte Payload-Bytes dieses Chunks
    juce::uint8 bytes[kPayloadBytes] = {};

    /** Anzahl Chunks, die eine Nachricht dieser Groesse belegt. */
    [[nodiscard]] static constexpr int chunksFor (int messageSize) noexcept
    {
        return (messageSize + kPayloadBytes - 1) / kPayloadBytes;
    }
};

static_assert (std::is_trivially_copyable_v<SysExChunk>,
               "SysExChunk reist durch die SpscQueue (memcpy-Semantik)");
static_assert (sizeof (SysExChunk) <= 64, "Chunk kompakt halten (Queue-Fenster)");

/** Obergrenze pro SysEx-Nachricht (ADR 007): der Mopho-Program-Dump ist
    299 Wire-Bytes — 1024 laesst Luft fuer verwandte DSI-Dumps, haelt aber
    Bulk-Fremdverkehr (Firmware-Updates o. ae.) aus den Queues. */
inline constexpr int kMaxSysExBytes = 1024;

} // namespace conduit::midi
