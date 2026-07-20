#pragma once

#include <juce_core/juce_core.h>

#include "Core/MacroBindings.h"

namespace conduit::loopermidi
{

//==============================================================================
/**
    MIDI-Map-Ziele der Looper-Page (07/2026): jedes mappbare Element wird
    als `grid::MacroControlKey` auf einer EIGENEN Layer-Ebene kodiert —
    die generische Learn-/Binding-Maschine (`grid::MidiInBindings`) bleibt
    unverändert, Grid- und Looper-Bindungen kollidieren nie.

    controlId-Packung (4-Bit-Felder reichen: Looper/Track 0..3,
    Slot/Send-Index 0..11): [Typ << 12 | looper << 8 | track << 4 | index].
    Track-Namen zählen GLOBAL im 4er-Raster (Konvention
    LooperPatchOutModule::globalTrackNumber — hier inline, Core darf keine
    Modules-Header ziehen, ADR 014). Pure, testbar.
*/

/** MacroControlKey::layer der Looper-Ziele (system=0, diy=1 sind Grid). */
inline constexpr int kLooperLayer = 2;

enum class Target : int
{
    slotCell = 0,    // looper, track, index = Slot
    segment,         // looper, index = 0..3 (8/4/2/1 Takte)
    sourceSelect,    // looper (Rising Edge = nächste Quelle)
    viewToggle,      // looper (FFT/WAVE)
    syncFree,        // looper
    lenPoti,         // looper (kontinuierlich, Knob-Norm)
    posPoti,         // looper
    reverse,         // looper
    vari,            // looper (kontinuierlich, ±2 Oktaven)
    tapeQuant,       // looper
    targetCycle,     // looper (TGT-Kurzklick)
    trackPlay,       // looper, track
    trackStop,       // looper, track
    mute,            // looper, track
    solo,            // looper, track
    sendLevel,       // looper, track, index = Send 0..3
    panX,            // looper, track (XY-X, kontinuierlich)
    panY,            // looper, track (XY-Y = Distanz)
    gain,            // looper, track (Meter-Zeile)
    masterToggle,    // global (MST im MIXER-Tab)
    stopAll,         // global
};

[[nodiscard]] constexpr int packControlId (Target type, int looper = 0,
                                           int track = 0, int index = 0) noexcept
{
    return (static_cast<int> (type) << 12)
         | ((looper & 0xF) << 8) | ((track & 0xF) << 4) | (index & 0xF);
}

[[nodiscard]] constexpr grid::MacroControlKey makeKey (Target type, int looper = 0,
                                                       int track = 0, int index = 0) noexcept
{
    return { kLooperLayer, packControlId (type, looper, track, index), 0 };
}

[[nodiscard]] constexpr Target targetOf (const grid::MacroControlKey& key) noexcept
{
    // Typ-Feld = ALLE Bits oberhalb 12 (21 Ziel-Typen > 4 Bit)
    return static_cast<Target> (key.controlId >> 12);
}

[[nodiscard]] constexpr int looperOf (const grid::MacroControlKey& key) noexcept
{
    return (key.controlId >> 8) & 0xF;
}

[[nodiscard]] constexpr int trackOf (const grid::MacroControlKey& key) noexcept
{
    return (key.controlId >> 4) & 0xF;
}

[[nodiscard]] constexpr int indexOf (const grid::MacroControlKey& key) noexcept
{
    return key.controlId & 0xF;
}

[[nodiscard]] constexpr bool isLooperKey (const grid::MacroControlKey& key) noexcept
{
    return key.layer == kLooperLayer;
}

/** Globale Track-Nummer im 4er-Raster (Konvention Patch-OUT-Kachel). */
[[nodiscard]] constexpr int globalTrack (const grid::MacroControlKey& key) noexcept
{
    return looperOf (key) * 4 + trackOf (key) + 1;
}

/** Anzeigename fürs Mappings-Listing („Play · Track 14", „LEN · Looper 2"). */
[[nodiscard]] inline juce::String targetName (const grid::MacroControlKey& key)
{
    const auto looperText = "Looper " + juce::String (looperOf (key) + 1);
    const auto trackText = "Track " + juce::String (globalTrack (key));
    const auto dot = juce::String::fromUTF8 (" \xc2\xb7 ");

    switch (targetOf (key))
    {
        case Target::slotCell:
            return "Slot " + juce::String (indexOf (key) + 1) + dot + trackText;
        case Target::segment:
        {
            static constexpr int bars[] = { 8, 4, 2, 1 };
            return "Commit " + juce::String (bars[indexOf (key) & 0x3])
                 + (bars[indexOf (key) & 0x3] == 1 ? " Bar" : " Bars") + dot + looperText;
        }
        case Target::sourceSelect: return "Source" + dot + looperText;
        case Target::viewToggle:   return "FFT/WAVE" + dot + looperText;
        case Target::syncFree:     return "Sync/Free" + dot + looperText;
        case Target::lenPoti:      return "LEN" + dot + looperText;
        case Target::posPoti:      return "POS" + dot + looperText;
        case Target::reverse:      return "Reverse" + dot + looperText;
        case Target::vari:         return "VARI" + dot + looperText;
        case Target::tapeQuant:    return "Tape/Quant" + dot + looperText;
        case Target::targetCycle:  return "TGT" + dot + looperText;
        case Target::trackPlay:    return "Play" + dot + trackText;
        case Target::trackStop:    return "Stop" + dot + trackText;
        case Target::mute:         return "Mute" + dot + trackText;
        case Target::solo:         return "Solo" + dot + trackText;
        case Target::sendLevel:
            return "Send " + juce::String (indexOf (key) + 1) + dot + trackText;
        case Target::panX:         return "Pan" + dot + trackText;
        case Target::panY:         return "Distance" + dot + trackText;
        case Target::gain:         return "Gain" + dot + trackText;
        case Target::masterToggle: return "MST";
        case Target::stopAll:      return "Stop All";
    }
    return "Looper";
}

} // namespace conduit::loopermidi
