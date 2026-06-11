#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/GraphManager.h"

namespace conduit
{

//==============================================================================
/**
    4×16-Grid-Editor eines StepSequencerModule — lebt im NodeComponent.

    Bindung ausschließlich an den ValueTree-Subtree (5.3): Drag/Touch in
    einer Zelle schreibt deren Step-Wert als paramValue in den Tree, der
    Parameter-Sync des GraphManager spiegelt aufs Audio-Atomic; OSC-/Undo-
    Änderungen kommen über den ValueTree-Listener zurück ins Bild.

    Playhead: 30-fps-Timer + transienter getModuleFor-Lookup (Muster
    ScopeDisplay) — im 4×16-Modus leuchtet die ganze Spalte, in den
    verketteten Modi die einzelne Zelle. Steps jenseits der eingestellten
    length werden abgedunkelt.
*/
class StepGridDisplay final : public juce::Component,
                              private juce::ValueTree::Listener,
                              private juce::Timer
{
public:
    StepGridDisplay (juce::ValueTree nodeTreeToBind, GraphManager& graphManagerToUse);
    ~StepGridDisplay() override;

    /** Teardown-Hook (Phase 1, 5.3): Rendering-Updates sofort stoppen. */
    void stopUpdates();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    void editCellAt (juce::Point<int> position);
    [[nodiscard]] juce::ValueTree parameterFor (const juce::String& parameterId) const;
    [[nodiscard]] float parameterValue (const juce::String& parameterId, double fallback) const;
    [[nodiscard]] juce::Rectangle<float> cellBounds (int row, int column) const;

    juce::ValueTree nodeTree;   // NUR der Subtree (5.3)
    GraphManager& graphManager;
    const juce::String nodeUuid;

    int highlightedCell = -1;   // row·16+col, -1 = kein Playhead bekannt

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGridDisplay)
};

} // namespace conduit
