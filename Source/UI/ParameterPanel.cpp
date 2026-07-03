#include "ParameterPanel.h"

namespace conduit
{

ParameterPanel::ParameterPanel (juce::ValueTree nodeTreeToBind)
    : nodeTree (std::move (nodeTreeToBind))
{
    nodeTree.addListener (this);
    buildRows();
}

ParameterPanel::~ParameterPanel()
{
    nodeTree.removeListener (this);
}

void ParameterPanel::stopUpdates()
{
    nodeTree.removeListener (this);
    setInterceptsMouseClicks (false, false);

    for (auto& row : rows)
        row->slider.setEnabled (false);
}

//==============================================================================
juce::ValueTree ParameterPanel::parametersTree() const
{
    return nodeTree.getChildWithName (id::parameters);
}

juce::ValueTree ParameterPanel::paramTreeFor (const juce::String& paramId) const
{
    return parametersTree().getChildWithProperty (id::paramId, paramId);
}

void ParameterPanel::buildRows()
{
    const auto parameters = parametersTree();

    for (int i = 0; i < parameters.getNumChildren(); ++i)
    {
        const auto param = parameters.getChild (i);

        auto row = std::make_unique<ParameterRow>();
        row->paramId = param.getProperty (id::paramId).toString();

        row->nameLabel.setText (row->paramId, juce::dontSendNotification);
        row->nameLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
        row->nameLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
        addAndMakeVisible (row->nameLabel);

        row->slider.setRange ((double) param.getProperty (id::paramMin, 0.0),
                              (double) param.getProperty (id::paramMax, 1.0), 0.0);
        row->slider.setValue ((double) param.getProperty (id::paramValue, 0.0),
                              juce::dontSendNotification);
        row->slider.setDoubleClickReturnValue (true, (double) param.getProperty (id::paramDefault, 0.0));

        // Schreibt NUR in den Tree — der GraphManager spiegelt auf das
        // Echtzeit-Atomic. Bewusst ohne UndoManager: Parameter-Sweeps sind
        // keine patchbaren Aktionen (gleiches Verhalten wie der OSC-Pfad 6.1).
        row->slider.onValueChange = [this, paramId = row->paramId, slider = &row->slider]
        {
            if (auto p = paramTreeFor (paramId); p.isValid())
                p.setProperty (id::paramValue, slider->getValue(), nullptr);
        };
        addAndMakeVisible (row->slider);

        rows.push_back (std::move (row));
    }
}

//==============================================================================
void ParameterPanel::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    // Slider folgt externen Quellen (OSC-Nachzug 6.1, Undo, Preset-Load) —
    // dontSendNotification verhindert die Rückkopplungsschleife.
    if (property != id::paramValue || ! tree.hasType (id::parameter))
        return;

    const auto paramId = tree.getProperty (id::paramId).toString();

    for (auto& row : rows)
        if (row->paramId == paramId)
        {
            row->slider.setValue ((double) tree.getProperty (id::paramValue), juce::dontSendNotification);
            return;
        }
}

//==============================================================================
juce::Rectangle<int> ParameterPanel::rowBounds (int index) const
{
    return { 0, index * rowHeight, getWidth(), rowHeight };
}

void ParameterPanel::resized()
{
    for (int i = 0; i < static_cast<int> (rows.size()); ++i)
    {
        auto area = rowBounds (i).reduced (0, 3);
        rows[static_cast<std::size_t> (i)]->nameLabel.setBounds (area.removeFromLeft (72));
        rows[static_cast<std::size_t> (i)]->slider.setBounds (area);
    }
}

} // namespace conduit
