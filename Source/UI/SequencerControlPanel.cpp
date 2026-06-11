#include "SequencerControlPanel.h"

#include "Modules/ConduitModule.h"

namespace conduit
{

namespace
{
    void styleCaption (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (11.0f)));
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
        label.setJustificationType (juce::Justification::centredLeft);
        label.setInterceptsMouseClicks (false, false);
    }
}

SequencerControlPanel::SequencerControlPanel (juce::ValueTree nodeTreeToBind)
    : nodeTree (std::move (nodeTreeToBind))
{
    nodeTree.addListener (this);

    //==========================================================================
    // Reihe 1 — Struktur
    modeBox.addItem (juce::String::fromUTF8 ("4\xc3\x97""16"), 1);   // 4×16
    modeBox.addItem (juce::String::fromUTF8 ("2\xc3\x97""32"), 2);   // 2×32
    modeBox.addItem (juce::String::fromUTF8 ("1\xc3\x97""64"), 3);   // 1×64
    modeBox.onChange = [this] { writeParameter ("mode", static_cast<float> (modeBox.getSelectedId() - 1)); };
    addAndMakeVisible (modeBox);
    styleCaption (modeLabel, "Mode");
    addAndMakeVisible (modeLabel);

    directionBox.addItem (juce::String::fromUTF8 ("Vorw\xc3\xa4rts"),  1);
    directionBox.addItem (juce::String::fromUTF8 ("R\xc3\xbc""ckw\xc3\xa4rts"), 2);
    directionBox.addItem ("Pendel", 3);
    directionBox.addItem ("Zufall", 4);
    directionBox.onChange = [this]
    {
        writeParameter ("direction", static_cast<float> (directionBox.getSelectedId() - 1));
    };
    addAndMakeVisible (directionBox);
    styleCaption (directionLabel, "Richtung");
    addAndMakeVisible (directionLabel);

    lengthSlider.setRange (1.0, 16.0, 1.0);
    lengthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 36, 24);
    lengthSlider.onValueChange = [this]
    {
        writeParameter ("length", static_cast<float> (lengthSlider.getValue()));
    };
    addAndMakeVisible (lengthSlider);
    styleCaption (lengthLabel, juce::String::fromUTF8 ("L\xc3\xa4nge"));
    addAndMakeVisible (lengthLabel);

    quantizeToggle.onClick = [this]
    {
        writeParameter ("quantize", quantizeToggle.getToggleState() ? 1.0f : 0.0f);
    };
    addAndMakeVisible (quantizeToggle);

    //==========================================================================
    // Reihe 2 — Drehregler
    bindKnob (rateKnob,  rateLabel,  "rate",  "Rate");
    bindKnob (gateKnob,  gateLabel,  "gate",  "Gate");
    bindKnob (swingKnob, swingLabel, "swing", "Swing");
    bindKnob (probKnob,  probLabel,  "prob",  "Prob");

    refreshAllControls();
}

SequencerControlPanel::~SequencerControlPanel()
{
    nodeTree.removeListener (this);
}

//==============================================================================
void SequencerControlPanel::bindKnob (juce::Slider& knob, juce::Label& label,
                                      const juce::String& parameterId, const juce::String& title)
{
    const auto parameter = parameterFor (parameterId);
    knob.setRange ((double) parameter.getProperty (id::paramMin, 0.0),
                   (double) parameter.getProperty (id::paramMax, 1.0), 0.0);
    knob.setDoubleClickReturnValue (true, (double) parameter.getProperty (id::paramDefault, 0.0));
    knob.onValueChange = [this, parameterId, &knob]
    {
        writeParameter (parameterId, static_cast<float> (knob.getValue()));
    };
    addAndMakeVisible (knob);

    styleCaption (label, title);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

//==============================================================================
void SequencerControlPanel::valueTreePropertyChanged (juce::ValueTree& tree,
                                                      const juce::Identifier& property)
{
    // Externe Quellen (OSC-Nachzug 6.1, Undo, Preset-Load) — Controls folgen,
    // dontSendNotification verhindert die Rückkopplungsschleife.
    if (property == id::paramValue && tree.hasType (id::parameter))
        refreshControl (tree.getProperty (id::paramId).toString());
}

void SequencerControlPanel::refreshControl (const juce::String& parameterId)
{
    if (parameterId == "mode")
        modeBox.setSelectedId (juce::jlimit (0, 2, juce::roundToInt (parameterValue ("mode", 0.0))) + 1,
                               juce::dontSendNotification);
    else if (parameterId == "direction")
        directionBox.setSelectedId (juce::jlimit (0, 3, juce::roundToInt (parameterValue ("direction", 0.0))) + 1,
                                    juce::dontSendNotification);
    else if (parameterId == "length")
        lengthSlider.setValue ((double) parameterValue ("length", 16.0), juce::dontSendNotification);
    else if (parameterId == "quantize")
        quantizeToggle.setToggleState (parameterValue ("quantize", 0.0) >= 0.5f,
                                       juce::dontSendNotification);
    else if (parameterId == "rate")
        rateKnob.setValue ((double) parameterValue ("rate", 2.0), juce::dontSendNotification);
    else if (parameterId == "gate")
        gateKnob.setValue ((double) parameterValue ("gate", 0.5), juce::dontSendNotification);
    else if (parameterId == "swing")
        swingKnob.setValue ((double) parameterValue ("swing", 0.0), juce::dontSendNotification);
    else if (parameterId == "prob")
        probKnob.setValue ((double) parameterValue ("prob", 1.0), juce::dontSendNotification);
}

void SequencerControlPanel::refreshAllControls()
{
    for (const auto* parameterId : { "mode", "direction", "length", "quantize",
                                     "rate", "gate", "swing", "prob" })
        refreshControl (parameterId);
}

//==============================================================================
void SequencerControlPanel::writeParameter (const juce::String& parameterId, float newValue)
{
    // Schreibt NUR in den Tree (ohne Undo — Parameter-Sweep wie Slider/OSC);
    // der GraphManager spiegelt aufs Echtzeit-Atomic
    if (auto parameter = parameterFor (parameterId); parameter.isValid())
        parameter.setProperty (id::paramValue, newValue, nullptr);
}

juce::ValueTree SequencerControlPanel::parameterFor (const juce::String& parameterId) const
{
    return nodeTree.getChildWithName (id::parameters)
               .getChildWithProperty (id::paramId, parameterId);
}

float SequencerControlPanel::parameterValue (const juce::String& parameterId, double fallback) const
{
    return static_cast<float> ((double) parameterFor (parameterId)
                                   .getProperty (id::paramValue, fallback));
}

//==============================================================================
void SequencerControlPanel::resized()
{
    auto bounds = getLocalBounds();

    //==========================================================================
    // Reihe 1 — Struktur: Caption (13px) über Control (44px)
    auto structureRow = bounds.removeFromTop (57);
    auto structureCaptions = structureRow.removeFromTop (13);

    const auto placeStructure = [&] (juce::Component& control, juce::Label* caption, int width)
    {
        control.setBounds (structureRow.removeFromLeft (width));
        if (caption != nullptr)
            caption->setBounds (structureCaptions.removeFromLeft (width));
        else
            structureCaptions.removeFromLeft (width);

        structureRow.removeFromLeft (8);
        structureCaptions.removeFromLeft (8);
    };

    placeStructure (modeBox,      &modeLabel,      80);
    placeStructure (directionBox, &directionLabel, 108);

    // Skala-Toggle rechts, der Längen-Slider bekommt den Rest
    quantizeToggle.setBounds (structureRow.removeFromRight (72));
    structureCaptions.removeFromRight (72);
    placeStructure (lengthSlider, &lengthLabel, structureRow.getWidth() - 8);

    bounds.removeFromTop (10);   // Lücke zwischen den Reihen

    //==========================================================================
    // Reihe 2 — Drehregler: vier gleiche Zellen, Caption unter dem Knob
    const auto cellWidth = bounds.getWidth() / 4;
    const std::pair<juce::Slider*, juce::Label*> knobs[] {
        { &rateKnob, &rateLabel }, { &gateKnob, &gateLabel },
        { &swingKnob, &swingLabel }, { &probKnob, &probLabel },
    };

    for (auto& [knob, caption] : knobs)
    {
        auto cell = bounds.removeFromLeft (cellWidth);
        caption->setBounds (cell.removeFromBottom (13));
        knob->setBounds (cell.withSizeKeepingCentre (44, juce::jmin (44, cell.getHeight())));
    }
}

} // namespace conduit
