#include "CapturePanel.h"

namespace conduit
{

namespace
{
    constexpr int rowHeight = 44;   // Touch-Target (CLAUDE.md 10)
    constexpr int gap = 8;

    const juce::Colour panelBackground { 0xff2a2e34 };
    const juce::Colour idleColour      { 0xff4a4f57 };
    const juce::Colour recordingColour { 0xffe6453c };
    const juce::Colour heldColour      { 0xffe6a23c };

    juce::Colour colourForState (CaptureChannel::State state)
    {
        switch (state)
        {
            case CaptureChannel::State::recording:
            case CaptureChannel::State::awaitingSegment: return recordingColour;
            case CaptureChannel::State::held:            return heldColour;
            case CaptureChannel::State::idle:            break;
        }
        return idleColour;
    }
}

//==============================================================================
CapturePanel::CapturePanel (CaptureSettings& settingsToUse, CaptureService& serviceToUse)
    : settings (settingsToUse), service (serviceToUse)
{
    // -- Schwelle / Hold: direkte Settings-Setter ------------------------------
    thresholdSlider.setRange (CaptureSettings::minThresholdDb,
                              CaptureSettings::maxThresholdDb, 1.0);
    thresholdSlider.setTextValueSuffix (" dB");
    thresholdSlider.onValueChange = [this]
    { settings.setThresholdDb (static_cast<float> (thresholdSlider.getValue())); };

    holdSlider.setRange (CaptureSettings::minHoldMinutes,
                         CaptureSettings::maxHoldMinutes, 1.0);
    holdSlider.setTextValueSuffix (" min Hold");
    holdSlider.onValueChange = [this]
    { settings.setHoldMinutes (juce::roundToInt (holdSlider.getValue())); };

    autoCalibrateToggle.onClick = [this]
    { settings.setAutoCalibrate (autoCalibrateToggle.getToggleState()); };

    // -- Ring-Dimensionierung: Resize-Policy (Settings-Doku) -------------------
    // Übernahme erst am Drag-Ende bzw. bei Textbox-Eingabe — sonst würde
    // jeder Drag-Tick eine Reallokation (oder den Bestätigungs-Dialog) treten
    preRollSlider.setRange (CaptureSettings::minPreRollSeconds,
                            CaptureSettings::maxPreRollSeconds, 5.0);
    preRollSlider.setTextValueSuffix (" s Pre-Roll");
    preRollSlider.onDragEnd = [this] { applyRingSlider (preRollSlider, false); };
    preRollSlider.onValueChange = [this]
    {
        if (! preRollSlider.isMouseButtonDown())  // Textbox/Keyboard
            applyRingSlider (preRollSlider, false);
    };

    bufferSlider.setRange (CaptureSettings::minBufferMinutes,
                           CaptureSettings::maxBufferMinutes, 5.0);
    bufferSlider.setTextValueSuffix (" min Ring");
    bufferSlider.onDragEnd = [this] { applyRingSlider (bufferSlider, true); };
    bufferSlider.onValueChange = [this]
    {
        if (! bufferSlider.isMouseButtonDown())
            applyRingSlider (bufferSlider, true);
    };

    // Bei aktiver Aufnahme bestätigt die UI async (13.2: keine Modal-Loops)
    settings.onPendingResize = [this] (const CaptureSettings::PendingResizeRequest& request)
    {
        syncControls();  // Slider zurück auf den aktiven Wert, solange offen

        const auto* fieldName =
            request.field == CaptureSettings::PendingResizeRequest::Field::bufferMinutes
                ? "Ring-Puffer" : "Pre-Roll";
        const auto message = juce::String (fieldName) + " auf "
                           + juce::String (request.requestedValue)
                           + juce::String::fromUTF8 (" \xc3\xa4ndern? Die laufende Aufnahme"
                                                     " wird verworfen (kein Auto-Export).");

        auto* settingsPtr = &settings;  // Settings überleben den Editor (Processor-Besitz)
        juce::AlertWindow::showOkCancelBox (
            juce::MessageBoxIconType::QuestionIcon,
            juce::String::fromUTF8 ("Capture-Puffer \xc3\xa4ndern?"), message,
            juce::String::fromUTF8 ("\xc3\x84ndern"), "Abbrechen", this,
            juce::ModalCallbackFunction::create ([settingsPtr] (int result)
            {
                if (result == 1)
                    settingsPtr->confirmPendingResize();
                else
                    settingsPtr->cancelPendingResize();
            }));
    };

    // -- Export-Ziel -----------------------------------------------------------
    for (const auto bits : { 16, 24, 32 })
        bitDepthCombo.addItem (juce::String (bits) + " Bit", bits);
    bitDepthCombo.onChange = [this]
    { settings.setExportBitDepth (bitDepthCombo.getSelectedId()); };

    directoryButton.onClick = [this] { chooseExportDirectory(); };

    directoryLabel.setColour (juce::Label::textColourId,
                              juce::Colours::white.withAlpha (0.6f));
    directoryLabel.setJustificationType (juce::Justification::centredLeft);
    directoryLabel.setMinimumHorizontalScale (0.7f);

    releaseAfterExportToggle.onClick = [this]
    { settings.setReleaseAfterExport (releaseAfterExportToggle.getToggleState()); };

    ramWarningLabel.setText ("RAM-Limit!", juce::dontSendNotification);
    ramWarningLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
    ramWarningLabel.setVisible (false);

    addAndMakeVisible (thresholdSlider);
    addAndMakeVisible (holdSlider);
    addAndMakeVisible (preRollSlider);
    addAndMakeVisible (bufferSlider);
    addAndMakeVisible (autoCalibrateToggle);
    addAndMakeVisible (releaseAfterExportToggle);
    addAndMakeVisible (bitDepthCombo);
    addAndMakeVisible (directoryButton);
    addAndMakeVisible (directoryLabel);
    addAndMakeVisible (ramWarningLabel);

    settings.addChangeListener (this);
    service.addChangeListener (this);   // RAM-Wächter-Warnung
    syncControls();
}

CapturePanel::~CapturePanel()
{
    settings.onPendingResize = nullptr;
    settings.removeChangeListener (this);
    service.removeChangeListener (this);
}

//==============================================================================
void CapturePanel::applyRingSlider (juce::Slider& slider, bool isBufferMinutes)
{
    const auto value = juce::roundToInt (slider.getValue());
    const auto outcome = isBufferMinutes ? settings.setBufferMinutes (value)
                                         : settings.setPreRollSeconds (value);

    // pendingConfirmation: onPendingResize hat den Dialog gestartet und
    // syncControls() den Slider zurückgesetzt — hier nichts weiter zu tun
    juce::ignoreUnused (outcome);
}

void CapturePanel::chooseExportDirectory()
{
    directoryChooser = std::make_unique<juce::FileChooser> (
        juce::String::fromUTF8 ("Export-Ordner w\xc3\xa4hlen"), settings.getExportDirectory());

    directoryChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this] (const juce::FileChooser& chooser)
        {
            if (chooser.getResult() != juce::File())
                settings.setExportDirectory (chooser.getResult());
        });
}

void CapturePanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Settings-Broadcast und RAM-Warnung laufen beide hier auf — ein
    // kompletter Sync ist billig und hält die Controls konsistent
    syncControls();
}

void CapturePanel::syncControls()
{
    thresholdSlider.setValue (settings.getThresholdDb(), juce::dontSendNotification);
    holdSlider.setValue (settings.getHoldMinutes(), juce::dontSendNotification);
    preRollSlider.setValue (settings.getPreRollSeconds(), juce::dontSendNotification);
    bufferSlider.setValue (settings.getBufferMinutes(), juce::dontSendNotification);
    autoCalibrateToggle.setToggleState (settings.getAutoCalibrate(), juce::dontSendNotification);
    releaseAfterExportToggle.setToggleState (settings.getReleaseAfterExport(),
                                             juce::dontSendNotification);
    bitDepthCombo.setSelectedId (settings.getExportBitDepth(), juce::dontSendNotification);
    directoryLabel.setText (settings.getExportDirectory().getFullPathName(),
                            juce::dontSendNotification);
    ramWarningLabel.setVisible (service.isRamWarningActive());
}

//==============================================================================
void CapturePanel::refresh()
{
    const auto numChannels = service.getRingNumChannels();
    bool changed = static_cast<int> (channelSnapshots.size()) != numChannels;
    channelSnapshots.resize (static_cast<size_t> (numChannels));

    const auto ringCapacity = service.getRingCapacitySamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* channel = service.getChannel (ch);
        if (channel == nullptr)
            continue;

        ChannelSnapshot snapshot;
        snapshot.state = channel->getState();

        if (snapshot.state != CaptureChannel::State::idle && ringCapacity > 0)
        {
            const auto range = channel->getReadableRange();
            const auto fill = static_cast<float> (range.to - range.from)
                            / static_cast<float> (ringCapacity);
            snapshot.fillSteps = juce::roundToInt (juce::jlimit (0.0f, 1.0f, fill) * 32.0f);
        }

        auto& previous = channelSnapshots[static_cast<size_t> (ch)];
        if (previous.state != snapshot.state || previous.fillSteps != snapshot.fillSteps)
        {
            previous = snapshot;
            changed = true;
        }
    }

    if (changed)
        repaint (channelArea);
}

//==============================================================================
void CapturePanel::paint (juce::Graphics& g)
{
    g.fillAll (panelBackground);

    // Kanal-Leiste: eine Säule pro Input — Statusfarbe, Höhe = Füllstand
    auto area = channelArea.reduced (4);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (area.toFloat(), 4.0f);

    const auto numChannels = static_cast<int> (channelSnapshots.size());
    if (numChannels == 0)
    {
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.setFont (juce::FontOptions { 13.0f });
        g.drawText (juce::String::fromUTF8 ("keine Eing\xc3\xa4nge"), area,
                    juce::Justification::centred);
        return;
    }

    const auto inner = area.reduced (6);
    const auto columnWidth = juce::jmax (3, juce::jmin (14, inner.getWidth() / numChannels));

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto& snapshot = channelSnapshots[static_cast<size_t> (ch)];
        auto column = juce::Rectangle<int> (inner.getX() + ch * columnWidth, inner.getY(),
                                            columnWidth - 2, inner.getHeight());
        if (column.getRight() > inner.getRight())
            break;  // mehr Kanäle als Platz — Rest mit dem Mixer-Meilenstein

        const auto colour = colourForState (snapshot.state);
        g.setColour (colour.withAlpha (0.25f));
        g.fillRect (column);

        const auto fillHeight = column.getHeight() * snapshot.fillSteps / 32;
        g.setColour (colour);
        g.fillRect (column.removeFromBottom (juce::jmax (
            snapshot.state == CaptureChannel::State::idle ? 0 : 2, fillHeight)));
    }
}

void CapturePanel::resized()
{
    auto bounds = getLocalBounds().reduced (8, 6);

    // Rechts: Kanal-Leiste (~28 %), links zwei Control-Zeilen
    channelArea = bounds.removeFromRight (bounds.getWidth() * 28 / 100);
    bounds.removeFromRight (gap);

    auto topRow = bounds.removeFromTop (rowHeight);
    bounds.removeFromTop (bounds.getHeight() - rowHeight);  // Rest-Lücke mittig
    auto bottomRow = bounds;

    const auto place = [] (juce::Rectangle<int>& row, juce::Component& component,
                           int width, int gapAfter = gap)
    {
        component.setBounds (row.removeFromLeft (width));
        row.removeFromLeft (gapAfter);
    };

    place (topRow, thresholdSlider, 190);
    place (topRow, autoCalibrateToggle, 130);
    place (topRow, holdSlider, 160);
    place (topRow, preRollSlider, 180);
    place (topRow, bufferSlider, 170);
    ramWarningLabel.setBounds (topRow);

    place (bottomRow, bitDepthCombo, 100);
    place (bottomRow, directoryButton, 100);
    place (bottomRow, releaseAfterExportToggle, 280);
    directoryLabel.setBounds (bottomRow);
}

} // namespace conduit
