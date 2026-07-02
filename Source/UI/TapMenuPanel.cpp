#include "TapMenuPanel.h"

#include "PushLookAndFeel.h"

namespace conduit
{

TapMenuPanel::TapMenuPanel (TransportSettings& settingsToUse)
    : settings (settingsToUse)
{
    autoCommitToggle.setToggleState (settings.isTapAutoCommitEnabled(),
                                     juce::dontSendNotification);
    autoCommitToggle.setTooltip (juce::String::fromUTF8 (
        "Ab Tap n committet jeder weitere Tap das Tempo zur Session — "
        "für gemappte Tap-Buttons (MIDI/OSC) ohne Set-Klick"));
    autoCommitToggle.onClick = [this]
    {
        settings.setTapAutoCommitEnabled (autoCommitToggle.getToggleState());
        tapCountSlider.setEnabled (autoCommitToggle.getToggleState());
    };

    tapCountCaption.setText ("Auto-Commit ab Tap n", juce::dontSendNotification);
    tapCountCaption.setColour (juce::Label::textColourId, push::colours::textDim);

    tapCountSlider.setRange (2.0, 8.0, 1.0);
    tapCountSlider.setValue (settings.getTapCount(), juce::dontSendNotification);
    tapCountSlider.setEnabled (settings.isTapAutoCommitEnabled());
    tapCountSlider.onValueChange = [this]
    { settings.setTapCount (juce::roundToInt (tapCountSlider.getValue())); };

    resetHoldCaption.setText ("Reset: Tap halten", juce::dontSendNotification);
    resetHoldCaption.setColour (juce::Label::textColourId, push::colours::textDim);

    resetHoldSlider.setRange (0.3, 3.0, 0.1);
    resetHoldSlider.setTextValueSuffix (" s");
    resetHoldSlider.setDoubleClickReturnValue (true, 1.0);
    resetHoldSlider.setValue (settings.getTapResetHoldSeconds(), juce::dontSendNotification);
    resetHoldSlider.onValueChange = [this]
    { settings.setTapResetHoldSeconds (resetHoldSlider.getValue()); };

    addAndMakeVisible (autoCommitToggle);
    addAndMakeVisible (tapCountCaption);
    addAndMakeVisible (tapCountSlider);
    addAndMakeVisible (resetHoldCaption);
    addAndMakeVisible (resetHoldSlider);

    setSize (panelWidth, panelHeight);
}

void TapMenuPanel::resized()
{
    auto bounds = getLocalBounds().reduced (12, 10);

    autoCommitToggle.setBounds (bounds.removeFromTop (28));
    bounds.removeFromTop (4);
    tapCountCaption.setBounds (bounds.removeFromTop (18));
    tapCountSlider.setBounds (bounds.removeFromTop (28));
    bounds.removeFromTop (8);
    resetHoldCaption.setBounds (bounds.removeFromTop (18));
    resetHoldSlider.setBounds (bounds.removeFromTop (28));
}

void TapMenuPanel::paint (juce::Graphics& g)
{
    g.fillAll (push::colours::panel);
}

} // namespace conduit
