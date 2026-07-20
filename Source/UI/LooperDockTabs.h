#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Looper/LooperMidiMap.h"
#include "Core/LooperSettings.h"
#include "EditorDockPanel.h"

namespace conduit
{

//==============================================================================
/**
    Seitenpanel-Tabs der Looper-Page (Mixer/Panel 07/2026): LOOPER ·
    MIXER · MIDI im app-weiten EditorDockPanel (Page-Maske = Looper-Page,
    Muster GridPage). Ersetzt das ⚙-CallOutBox-Menü (LooperSettingsMenu).

    - LOOPER-Tab: LAYOUT (Looper-/Track-Anzahl −/+, Delete-Gating läuft
      über die Editor-Hooks) + LOOPER · GLOBAL (alle Verhaltens-Optionen,
      englische Labels; ÷2-Hälfte entfällt — LEN/POS-Potis decken das ab,
      User-Entscheidung 20.07.2026).
    - MIXER-/MIDI-Tab: Platzhalter, gefüllt von den Folge-Meilensteinen
      (MASTER/SENDS/DISTANCE/DISPLAY bzw. MAP MODE).

    Bindet DIREKT an die LooperSettings (App-Zustand) und hört als
    ChangeListener mit — externe Änderungen (Editor-Refresh, Migration)
    spiegeln sich in die Controls. Struktur-Änderungen (Looper/Tracks)
    laufen über die Editor-Hooks, NIE direkt in die Settings (Delete-
    Gating, ADR 012). Lebensdauer: Tabs werden im Ctor registriert und
    im Dtor entfernt — Member NACH dem editorDock deklarieren
    (EngineEditor-Muster GridPage).
*/
class LooperDockTabs final : private juce::ChangeListener
{
public:
    LooperDockTabs (EditorDockPanel& dockToUse, LooperSettings& settingsToUse,
                    LooperMidiMap& midiMapToUse);
    ~LooperDockTabs() override;

    //==========================================================================
    // LAYOUT-Hooks [Editor] — Struktur-Pfade mit Delete-Gating
    std::function<void()> onAddLooper;
    std::function<void()> onRemoveLooper;
    std::function<void (int looperIndex)> onAddTrack;
    std::function<void (int looperIndex)> onRemoveTrack;

    //==========================================================================
    // MIXER · MASTER [Editor] — Output-Paar (aus der alten Kopfzeile) +
    // globaler MST-Toggle (setzt sendMaster ALLER Looper, User 20.07.2026)

    std::function<void (int pairIndex)> onOutputPairSelected;   // −1 = Kein Master-Out
    std::function<void (bool toMaster)> onMasterToggled;

    void setOutputPairs (const juce::StringArray& pairLabels, int selectedPair);
    void setMasterState (bool allLoopersToMaster);

    /** [Editor, refreshLooperStructure] LAYOUT-Zeilen an die aktuelle
        Struktur anpassen (Zähler, Enable-Zustand der −/+-Kacheln). */
    void refreshLayout();

    //==========================================================================
    // MIDI-Tab (MAP MODE, 07/2026)

    /** Feuert beim Umschalten des MAP-MODE-Toggles (Editor blendet das
        Overlay um). */
    std::function<void (bool active)> onMapModeChanged;

    /** Learn-Kachel der Mappings-Liste bzw. Overlay-Tap (Editor armiert). */
    std::function<void (const grid::MacroControlKey&)> onLearnRequested;

    [[nodiscard]] bool isMapModeActive() const noexcept;
    void setMapModeActive (bool active);

    /** Mappings-Liste neu aufbauen (nach Binding-Änderungen). */
    void refreshMappings();

    /** Learn-scharfe Zeile markieren (hasArmed=false löscht). */
    void setArmedKey (bool hasArmed, const grid::MacroControlKey& key);

    /** Content des LOOPER-/MIXER-Tabs (Tests: Controls via ComponentID). */
    [[nodiscard]] juce::Component* getLooperTabContent() noexcept;
    [[nodiscard]] juce::Component* getMixerTabContent() noexcept;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    class LooperTabView;
    class MixerTabView;
    class MidiTabView;

    EditorDockPanel& dock;
    LooperSettings& settings;
    LooperMidiMap& midiMap;

    // Vom Dock besessen (addTab) — Pointer gültig bis removeTab im Dtor
    LooperTabView* looperView = nullptr;
    MixerTabView* mixerView = nullptr;
    MidiTabView* midiView = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperDockTabs)
};

} // namespace conduit
