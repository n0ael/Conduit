#include "NodeAttributePanel.h"

#include "UI/PushLookAndFeel.h"

namespace conduit
{

namespace
{
    constexpr int panelWidth = 280;
    constexpr int pad        = 16;
    constexpr int rowH       = 26;
}

//==============================================================================
NodeAttributePanel::NodeAttributePanel (const juce::String& currentNameToUse,
                                        juce::uint32 currentColour)
    : currentName (currentNameToUse)
{
    nameCaption.setText (juce::String::fromUTF8 ("Name"), juce::dontSendNotification);
    nameCaption.setFont (push::scaledFont (11.0f));
    nameCaption.setColour (juce::Label::textColourId, push::colours::textDim);
    addAndMakeVisible (nameCaption);

    nameEditor.setText (currentName, juce::dontSendNotification);
    nameEditor.setEditable (true, true, false);
    nameEditor.setColour (juce::Label::backgroundColourId, push::colours::tile);
    nameEditor.setColour (juce::Label::outlineColourId, push::colours::outline);
    nameEditor.setColour (juce::Label::textColourId, push::colours::text);
    nameEditor.onTextChange = [this]
    {
        const auto requested = nameEditor.getText();

        // Ablehnung (Name vergeben/ungültig) → Feld zurück auf den echten Namen
        if (onRename != nullptr && ! onRename (requested))
            nameEditor.setText (currentName, juce::dontSendNotification);
        else
            currentName = nameEditor.getText();
    };
    addAndMakeVisible (nameEditor);

    colourCaption.setText (juce::String::fromUTF8 ("Farbe"), juce::dontSendNotification);
    colourCaption.setFont (push::scaledFont (11.0f));
    colourCaption.setColour (juce::Label::textColourId, push::colours::textDim);
    addAndMakeVisible (colourCaption);

    colourStrip.setSelected (currentColour);
    colourStrip.onColourChosen = [this] (juce::uint32 colour)
    {
        if (onColour != nullptr)
            onColour (colour);
    };
    addAndMakeVisible (colourStrip);

    setSize (panelWidth,
             pad + rowH + 30                                    // Name-Caption + Feld
                 + rowH + ColourSwatchStrip::preferredHeight()   // Farbe-Caption + Strip
                 + pad);
}

//==============================================================================
void NodeAttributePanel::paint (juce::Graphics& g)
{
    g.fillAll (push::colours::panel);
}

void NodeAttributePanel::resized()
{
    auto bounds = getLocalBounds().reduced (pad, pad);

    nameCaption.setBounds (bounds.removeFromTop (rowH).withTrimmedBottom (8));
    nameEditor.setBounds (bounds.removeFromTop (30));

    colourCaption.setBounds (bounds.removeFromTop (rowH).withTrimmedBottom (8));
    colourStrip.setBounds (bounds.removeFromTop (ColourSwatchStrip::preferredHeight())
                               .withWidth (ColourSwatchStrip::preferredWidth()));
}

} // namespace conduit
