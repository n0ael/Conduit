#pragma once

#include <functional>

#include <juce_data_structures/juce_data_structures.h>

#include "Core/MidiInBindings.h"
#include "LooperMidiTargets.h"

namespace conduit
{

//==============================================================================
/**
    MIDI-Map der Looper-Page (07/2026): besitzt eine EIGENE
    `grid::MidiInBindings`-Instanz (Learn/Soft-Takeover/Glättung gratis,
    komplett getrennt von den Grid-Bindings) und persistiert die
    Bindungen in einer eigenen Datei (Conduit/LooperMidi.settings,
    Muster LooperSettings — App-Zustand, kein Patch).

    Der EngineEditor pumpt Controller-Events aller Controller-Rolle-
    Geräte hinein (MidiPortHub-Drain) und wendet Werte über den
    tick()-Dispatch an; die MappingsListComponent des MIDI-Tabs bindet
    direkt an getBindings(). Message Thread.
*/
class LooperMidiMap
{
public:
    [[nodiscard]] static juce::PropertiesFile::Options defaultOptions();

    explicit LooperMidiMap (const juce::PropertiesFile::Options& options = defaultOptions());
    ~LooperMidiMap();

    [[nodiscard]] grid::MidiInBindings& getBindings() noexcept { return bindings; }

    /** Feuert nach jeder Struktur-Änderung (bind/unbind — nach dem
        Speichern); die UI baut Liste + Overlay-Badges neu. */
    std::function<void()> onChanged;

    /** [Tests] Ausstehende Änderungen sofort auf Platte schreiben. */
    void flush();

private:
    void loadFromFile();
    void saveToFile();

    juce::ApplicationProperties applicationProperties;
    grid::MidiInBindings bindings;
    bool loading = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperMidiMap)
};

} // namespace conduit
