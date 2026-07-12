#pragma once

#include <juce_core/juce_core.h>

namespace conduit::CcNames
{

//==============================================================================
/** Block L (Masterplan): Funktionsnamen der ~25 gebräuchlichsten General-
    MIDI-/MPE-CC-Nummern — reine Daten + Lookup, kein Zustand, kein neues
    Persistenz-Format. Unbekannte Nummern fallen auf "CC {n}" zurück (bisheriges
    Format), daher braucht kein Aufrufer eine Sonderbehandlung. Header-only,
    headless testbar. */
struct Entry
{
    int cc;
    const char* name;
};

inline constexpr Entry kTable[] = {
    { 1,   "Mod Wheel" },
    { 2,   "Breath" },
    { 4,   "Foot Ctrl" },
    { 5,   "Portamento Time" },
    { 7,   "Volume" },
    { 8,   "Balance" },
    { 10,  "Pan" },
    { 11,  "Expression" },
    { 64,  "Sustain" },
    { 65,  "Portamento" },
    { 66,  "Sostenuto" },
    { 67,  "Soft Pedal" },
    { 71,  "Resonance" },
    { 72,  "Release" },
    { 73,  "Attack" },
    { 74,  "Brightness" },   // MPE: Timbre/Slide
    { 75,  "Slide/Timbre" },
    { 84,  "Portamento Ctrl" },
    { 91,  "Reverb Send" },
    { 93,  "Chorus Send" },
    { 94,  "Detune" },
    { 98,  "NRPN LSB" },
    { 99,  "NRPN MSB" },
    { 100, "RPN LSB" },
    { 101, "RPN MSB" },
    { 123, "All Notes Off" },
};

/** "Mod Wheel" für bekannte CCs, sonst der bisherige Fallback "CC {n}". */
[[nodiscard]] inline juce::String displayName (int cc)
{
    for (const auto& entry : kTable)
        if (entry.cc == cc)
            return entry.name;

    return "CC " + juce::String (cc);
}

} // namespace conduit::CcNames
