#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Urzwerg-artige Kontrollleiste eines StepSequencerModule — lebt im
    NodeComponent unterhalb des StepGridDisplay.

    Zwei Reihen (analog zur Bedienleiste der Hardware):
      Reihe 1 — Struktur:  Mode (4×16/2×32/1×64), Richtung, Länge, Skala-Toggle
      Reihe 2 — Regler:    Rate, Gate, Swing, Prob als Drehregler

    Bindung ausschließlich an den ValueTree-Subtree (5.3): jede Eingabe
    schreibt paramValue in den Tree (ohne Undo — Parameter-Sweep wie
    Slider/OSC, 6.1), der GraphManager spiegelt aufs Audio-Atomic.
    OSC-/Undo-/Preset-Änderungen kommen über den ValueTree-Listener zurück.

    Touch-first (10): alle Controls 44px hoch, Drehregler 44×44.
*/
class SequencerControlPanel final : public juce::Component,
                                    private juce::ValueTree::Listener
{
public:
    explicit SequencerControlPanel (juce::ValueTree nodeTreeToBind);
    ~SequencerControlPanel() override;

    static constexpr int preferredHeight = 124;   // 2 Reihen à 57 + Lücke

    void resized() override;

    // Controls öffentlich — die UI-Tests treiben sie direkt (sendNotificationSync)
    juce::ComboBox modeBox;
    juce::ComboBox directionBox;
    juce::Slider lengthSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::ToggleButton quantizeToggle { "Skala" };
    juce::Slider rateKnob  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider gateKnob  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider swingKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider probKnob  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

private:
    //==========================================================================
    // juce::ValueTree::Listener [Message Thread]
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    void bindKnob (juce::Slider& knob, juce::Label& label,
                   const juce::String& parameterId, const juce::String& title);
    void refreshControl (const juce::String& parameterId);
    void refreshAllControls();

    void writeParameter (const juce::String& parameterId, float newValue);
    [[nodiscard]] juce::ValueTree parameterFor (const juce::String& parameterId) const;
    [[nodiscard]] float parameterValue (const juce::String& parameterId, double fallback) const;

    juce::ValueTree nodeTree;   // NUR der Subtree (5.3)

    juce::Label modeLabel, directionLabel, lengthLabel;
    juce::Label rateLabel, gateLabel, swingLabel, probLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerControlPanel)
};

} // namespace conduit
