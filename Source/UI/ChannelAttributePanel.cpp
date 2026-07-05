#include "ChannelAttributePanel.h"

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
ChannelAttributePanel::ChannelAttributePanel (ChannelNames& channelNamesToUse,
                                              const InputLinkSend* sendServiceToUse,
                                              int portIndexToUse,
                                              bool hasNextNeighbourToUse)
    : channelNames (channelNamesToUse),
      portIndex (portIndexToUse),
      hasNextNeighbour (hasNextNeighbourToUse)
{
    const auto dir = ChannelNames::Direction::input;

    titleLabel.setText (channelNames.getLabel (dir, portIndex), juce::dontSendNotification);
    titleLabel.setFont (push::scaledFont (15.0f, true));
    titleLabel.setColour (juce::Label::textColourId, push::colours::text);
    addAndMakeVisible (titleLabel);

    nameCaption.setText (juce::String::fromUTF8 ("Name"), juce::dontSendNotification);
    nameCaption.setFont (push::scaledFont (11.0f));
    nameCaption.setColour (juce::Label::textColourId, push::colours::textDim);
    addAndMakeVisible (nameCaption);

    nameEditor.setText (channelNames.getUserLabel (dir, portIndex), juce::dontSendNotification);
    nameEditor.setEditable (true, true, false);
    nameEditor.setColour (juce::Label::backgroundColourId, push::colours::tile);
    nameEditor.setColour (juce::Label::outlineColourId, push::colours::outline);
    nameEditor.setColour (juce::Label::textColourId, push::colours::text);
    nameEditor.onTextChange = [this]
    {
        channelNames.setUserLabel (ChannelNames::Direction::input, portIndex,
                                   nameEditor.getText());
        // Effektives Label im Titel nachziehen (Default, wenn geleert)
        titleLabel.setText (channelNames.getLabel (ChannelNames::Direction::input, portIndex),
                            juce::dontSendNotification);
    };
    addAndMakeVisible (nameEditor);

    colourCaption.setText (juce::String::fromUTF8 ("Farbe"), juce::dontSendNotification);
    colourCaption.setFont (push::scaledFont (11.0f));
    colourCaption.setColour (juce::Label::textColourId, push::colours::textDim);
    addAndMakeVisible (colourCaption);

    colourStrip.setSelected (channelNames.getColour (dir, portIndex));
    colourStrip.onColourChosen = [this] (juce::uint32 colour)
    {
        channelNames.setColour (ChannelNames::Direction::input, portIndex, colour);
    };
    addAndMakeVisible (colourStrip);

    if (hasNextNeighbour)
    {
        stereoToggle.setButtonText (juce::String::fromUTF8 ("Mit nächstem Kanal koppeln (Stereo)"));
        stereoToggle.setToggleState (channelNames.isPortPairStart (dir, portIndex),
                                     juce::dontSendNotification);
        stereoToggle.onClick = [this]
        {
            channelNames.setPortPairedWithNext (ChannelNames::Direction::input, portIndex,
                                                stereoToggle.getToggleState());
        };
        addAndMakeVisible (stereoToggle);
    }

    sendCaption.setText (juce::String::fromUTF8 ("Link-Send"), juce::dontSendNotification);
    sendCaption.setFont (push::scaledFont (11.0f));
    sendCaption.setColour (juce::Label::textColourId, push::colours::textDim);
    addAndMakeVisible (sendCaption);

    sendButton = std::make_unique<InputSendButton> (channelNames, sendServiceToUse, portIndex);
    addAndMakeVisible (*sendButton);

    const auto stereoRows = hasNextNeighbour ? 1 : 0;
    setSize (panelWidth,
             pad + rowH                                     // Titel
                 + rowH + 30                                 // Name-Caption + Feld
                 + rowH + ColourSwatchStrip::preferredHeight() + 6  // Farbe-Caption + Strip
                 + stereoRows * (rowH + 4)                   // Stereo-Toggle
                 + rowH                                      // Send-Zeile
                 + pad);
}

ChannelAttributePanel::~ChannelAttributePanel()
{
    if (sendButton != nullptr)
        sendButton->stopUpdates();
}

//==============================================================================
void ChannelAttributePanel::paint (juce::Graphics& g)
{
    g.fillAll (push::colours::panel);
}

void ChannelAttributePanel::resized()
{
    auto bounds = getLocalBounds().reduced (pad, pad);

    titleLabel.setBounds (bounds.removeFromTop (rowH));

    nameCaption.setBounds (bounds.removeFromTop (rowH).withTrimmedBottom (8));
    nameEditor.setBounds (bounds.removeFromTop (30));

    colourCaption.setBounds (bounds.removeFromTop (rowH).withTrimmedBottom (8));
    colourStrip.setBounds (bounds.removeFromTop (ColourSwatchStrip::preferredHeight())
                               .withWidth (ColourSwatchStrip::preferredWidth()));
    bounds.removeFromTop (6);

    if (hasNextNeighbour)
    {
        stereoToggle.setBounds (bounds.removeFromTop (rowH));
        bounds.removeFromTop (4);
    }

    auto sendRow = bounds.removeFromTop (rowH);
    sendButton->setBounds (sendRow.removeFromLeft (28).withSizeKeepingCentre (24, 20));
    sendCaption.setBounds (sendRow.withTrimmedLeft (6));
}

} // namespace conduit
