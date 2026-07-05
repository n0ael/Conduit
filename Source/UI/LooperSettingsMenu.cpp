#include "LooperSettingsMenu.h"

#include "Core/LaunchQuantization.h"

namespace conduit
{

namespace
{
    constexpr int rowHeight = 34;
    constexpr int captionWidth = 150;
    constexpr int menuWidth = 340;
    constexpr int padding = 10;
}

LooperSettingsMenu::LooperSettingsMenu (LooperSettings& settingsToUse)
    : settings (settingsToUse)
{
    setName ("looperSettingsMenu");

    // Launch-Quantisierung (Start/Stop — Commit bleibt sofort)
    for (int i = 0; i < numLaunchQuants; ++i)
        quantCombo.addItem (launchQuantName (static_cast<LaunchQuant> (i)), i + 1);
    quantCombo.setSelectedId ((int) settings.getLaunchQuant() + 1,
                              juce::dontSendNotification);
    quantCombo.onChange = [this]
    {
        settings.setLaunchQuant (static_cast<LaunchQuant> (
            quantCombo.getSelectedItemIndex()));
    };

    tapModeCombo.addItem ("Retrigger", 1);
    tapModeCombo.addItem ("Stoppen", 2);
    tapModeCombo.setSelectedId (settings.getTapMode() == LooperSettings::TapMode::retrigger
                                    ? 1 : 2,
                                juce::dontSendNotification);
    tapModeCombo.onChange = [this]
    {
        settings.setTapMode (tapModeCombo.getSelectedId() == 1
                                 ? LooperSettings::TapMode::retrigger
                                 : LooperSettings::TapMode::toggleStop);
    };

    halveCombo.addItem (juce::String::fromUTF8 ("Erste Hälfte"), 1);
    halveCombo.addItem (juce::String::fromUTF8 ("Aktuelle Hälfte"), 2);
    halveCombo.setSelectedId (settings.getHalveMode() == looper::HalveMode::firstHalf
                                  ? 1 : 2,
                              juce::dontSendNotification);
    halveCombo.onChange = [this]
    {
        settings.setHalveMode (halveCombo.getSelectedId() == 1
                                   ? looper::HalveMode::firstHalf
                                   : looper::HalveMode::currentHalf);
    };

    reverseCombo.addItem ("Sofort", 1);
    reverseCombo.addItem ("An der Loop-Grenze", 2);
    reverseCombo.setSelectedId (settings.getReverseMode()
                                        == LooperSettings::ReverseMode::immediate
                                    ? 1 : 2,
                                juce::dontSendNotification);
    reverseCombo.onChange = [this]
    {
        settings.setReverseMode (reverseCombo.getSelectedId() == 1
                                     ? LooperSettings::ReverseMode::immediate
                                     : LooperSettings::ReverseMode::boundary);
    };

    variRasterCombo.addItem (juce::String::fromUTF8 ("Halbtöne"), 1);
    variRasterCombo.addItem ("Session-Skala", 2);
    variRasterCombo.setSelectedId (settings.getVariRaster()
                                           == LooperSettings::VariRaster::semitones
                                       ? 1 : 2,
                                   juce::dontSendNotification);
    variRasterCombo.onChange = [this]
    {
        settings.setVariRaster (variRasterCombo.getSelectedId() == 1
                                    ? LooperSettings::VariRaster::semitones
                                    : LooperSettings::VariRaster::sessionScale);
    };

    variScopeCombo.addItem ("Pro Track", 1);
    variScopeCombo.addItem ("Pro Looper", 2);
    variScopeCombo.setSelectedId (settings.getVariScope()
                                          == LooperSettings::VariScope::perTrack
                                      ? 1 : 2,
                                  juce::dontSendNotification);
    variScopeCombo.onChange = [this]
    {
        settings.setVariScope (variScopeCombo.getSelectedId() == 1
                                   ? LooperSettings::VariScope::perTrack
                                   : LooperSettings::VariScope::perLooper);
    };

    soloScopeCombo.addItem ("Pro Looper", 1);
    soloScopeCombo.addItem ("Global", 2);
    soloScopeCombo.setSelectedId (settings.getSoloScope()
                                          == LooperSettings::SoloScope::perLooper
                                      ? 1 : 2,
                                  juce::dontSendNotification);
    soloScopeCombo.onChange = [this]
    {
        settings.setSoloScope (soloScopeCombo.getSelectedId() == 1
                                   ? LooperSettings::SoloScope::perLooper
                                   : LooperSettings::SoloScope::globalScope);
    };

    for (int slots = LooperSettings::minVisibleSlots;
         slots <= LooperSettings::maxVisibleSlots; ++slots)
        slotsCombo.addItem (juce::String (slots), slots);
    slotsCombo.setSelectedId (settings.getVisibleSlots(), juce::dontSendNotification);
    slotsCombo.onChange = [this]
    { settings.setVisibleSlots (slotsCombo.getSelectedId()); };

    deleteLatchToggle.setToggleState (settings.isDeleteLatchEnabled(),
                                      juce::dontSendNotification);
    deleteLatchToggle.onClick = [this]
    { settings.setDeleteLatchEnabled (deleteLatchToggle.getToggleState()); };

    autoAdvanceToggle.setToggleState (settings.isAutoAdvanceEnabled(),
                                      juce::dontSendNotification);
    autoAdvanceToggle.onClick = [this]
    { settings.setAutoAdvanceEnabled (autoAdvanceToggle.getToggleState()); };

    addRow ("Quantisierung", quantCombo);
    addRow ("Tap auf Clip", tapModeCombo);
    addRow (juce::String::fromUTF8 ("÷2-Hälfte"), halveCombo);
    addRow ("Reverse", reverseCombo);
    addRow ("VARI-Raster", variRasterCombo);
    addRow ("VARI-Schalter", variScopeCombo);
    addRow ("Solo", soloScopeCombo);
    addRow ("Sichtbare Slots", slotsCombo);
    addRow ("Delete als Schalter", deleteLatchToggle);
    addRow ("Auto-Advance", autoAdvanceToggle);

    setSize (menuWidth,
             padding * 2 + rowHeight * (int) rows.size());
}

void LooperSettingsMenu::addRow (const juce::String& caption, juce::Component& control)
{
    auto row = std::make_unique<Row>();
    row->caption.setText (caption, juce::dontSendNotification);
    row->caption.setColour (juce::Label::textColourId, push::colours::textDim);
    row->caption.setJustificationType (juce::Justification::centredLeft);
    row->control = &control;

    addAndMakeVisible (row->caption);
    addAndMakeVisible (control);
    rows.push_back (std::move (row));
}

void LooperSettingsMenu::resized()
{
    auto bounds = getLocalBounds().reduced (padding);

    for (auto& row : rows)
    {
        auto line = bounds.removeFromTop (rowHeight);
        row->caption.setBounds (line.removeFromLeft (captionWidth));
        row->control->setBounds (line.reduced (0, 4));
    }
}

void LooperSettingsMenu::paint (juce::Graphics& g)
{
    g.fillAll (push::colours::panel);
}

} // namespace conduit
