#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_osc/juce_osc.h>

#include "TouchLive/LiveSetModel.h"

namespace conduit
{

//==============================================================================
/**
    Ableton-Track-Selector (Block H, Masterplan): klappt beim Long-Press auf
    den Grid-Page-Button als CallOutBox aus und listet alle MIDI-Tracks des
    Live-Sets mit Name + Live-Farbe (tracks-Domain des LiveSetModel,
    kind == "midi", sortiert nach Live-Reihenfolge).

    Tap auf einen Track meldet onTrackChosen (Stable-ID) — der EngineEditor
    sendet daraufhin /live/song/set/midi_input_focus: der gewählte Track
    bekommt Monitor „In" + Conduits Grid-MIDI-Out als Input („Conduit A"),
    alle anderen MIDI-Tracks Monitor „Auto" + das in den Grid-Settings
    gewählte MIDI-Eingabegerät (leer = Routing unangetastet). Zweck:
    unabhängig von Push/Keyboard jeden Kanal direkt aus Conduit spielen.

    Die Zeilen sind ein Snapshot beim Öffnen (die CallOutBox lebt nur einen
    Moment — kein Listener nötig). Stable-IDs sind Laufzeit-IDs der
    Gegenseite und werden NIE serialisiert (CLAUDE.md §6). Message Thread.
*/
class TrackSelectorPanel final : public juce::Component
{
public:
    struct TrackRow
    {
        juce::String key;      // Stable-ID der Gegenseite ("tr:…")
        juce::String name;
        juce::Colour colour;   // Live-Farbe 0x00RRGGBB
        int index = 0;         // Live-Reihenfolge
    };

    explicit TrackSelectorPanel (LiveSetModel& model);

    /** Track angetippt (Stable-ID) — der Besitzer sendet das Command und
        die Box schließt sich selbst. */
    std::function<void (const juce::String& stableKey)> onTrackChosen;

    //==========================================================================
    // Headless-Kernpfade (Catch2)

    /** MIDI-Tracks (kind == "midi") der tracks-Domain, nach index sortiert. */
    [[nodiscard]] static std::vector<TrackRow> midiTrackRowsFrom (LiveSetModel& model);

    /** /live/song/set/midi_input_focus [trackKey, inputName, defaultInputName]. */
    [[nodiscard]] static juce::OSCMessage
        makeMidiInputFocusCommand (const juce::String& trackKey,
                                   const juce::String& inputName,
                                   const juce::String& defaultInputName);

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    static constexpr int kRowHeight   = 44;   // Touch-Zone (CLAUDE.md 10.0)
    static constexpr int kTitleHeight = 30;
    static constexpr int kPanelWidth  = 280;

private:
    [[nodiscard]] int rowIndexAt (juce::Point<int> position) const noexcept;

    std::vector<TrackRow> rows;
    int hoveredRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackSelectorPanel)
};

} // namespace conduit
