#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/GridVoiceEngine.h"
#include "Core/MidiDeviceTarget.h"
#include "ExpressionRibbon.h"
#include "GridKeyboardComponent.h"
#include "PushTiles.h"

namespace conduit
{

//==============================================================================
/**
    Grid-Page (Ω, M1 Teil 3 — erster spielbarer Ton; M1b-3 — globale
    Ausdrucksebene + Panic; M1b-4 — bipolarer Pressure-Offset):
    GridKeyboardComponent (Hauptfläche) flankiert von zwei Rand-Ribbons —
    links Volume (unipolar, MPE-Master-Kanal CC7, GridVoiceEngine::
    setGlobalVolume), rechts AT-Offset (bipolar, Mitte = neutral, wirkt als
    Offset auf jeden per-Voice-Pressure, GridVoiceEngine::setPressureOffset)
    — plus ein minimales MIDI-Out-Port-Dropdown und ein Release-All-Button
    (GridVoiceEngine::allNotesOff). Kein Feinschliff darüber hinaus — reine
    Funktion, bis der Touch-Controller-Baukasten (CLAUDE.md 14 ADR) als
    eigener Meilenstein folgt.
*/
class GridPage final : public juce::Component
{
public:
    GridPage (grid::GridVoiceEngine& engineToUse, grid::MidiDeviceTarget& midiTargetToUse);

    void resized() override;

private:
    void rebuildDeviceList();
    void handleDeviceSelected();

    grid::GridVoiceEngine& engine;
    grid::MidiDeviceTarget& midiTarget;
    juce::Array<juce::MidiDeviceInfo> devices;

    juce::ComboBox outputCombo;
    push::TextTile releaseAllButton { "Release All", push::colours::ledRed };
    ExpressionRibbon volumeRibbon    { "VOL" };
    ExpressionRibbon atOffsetRibbon  { "AT", true }; // bipolar
    GridKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridPage)
};

} // namespace conduit
