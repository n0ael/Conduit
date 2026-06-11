#include "EngineEditor.h"

#include "EngineProcessor.h"
#include "Modules/AttenuatorModule.h"
#include "Modules/LfoModule.h"
#include "Modules/ScopeModule.h"

namespace conduit
{

EngineEditor::EngineEditor (EngineProcessor& engineProcessor)
    : juce::AudioProcessorEditor (engineProcessor),
      engine (engineProcessor),
      rootState (engineProcessor.getRootState()),
      undoManager (engineProcessor.getUndoManager()),
      graphManager (engineProcessor.getGraphManager()),
      linkClock (engineProcessor.getLinkClock()),
      canvas (rootState, engineProcessor.getGraphManager(), engineProcessor.getNodeUiRegistry())
{
    const auto addModule = [this] (const char* moduleId)
    {
        // Versetzt platzieren, damit gestapelte Nodes greifbar bleiben
        const auto offset = 24 * (canvas.getNumNodeComponents() % 8);
        const auto created = graphManager.addModuleNode (moduleId, { 40 + offset, 40 + offset });
        jassertquiet (created.isValid());
    };

    addButton.onClick      = [addModule] { addModule (AttenuatorModule::staticModuleId); };
    addLfoButton.onClick   = [addModule] { addModule (LfoModule::staticModuleId); };
    addScopeButton.onClick = [addModule] { addModule (ScopeModule::staticModuleId); };

    undoButton.onClick = [this] { undoManager.undo(); };
    redoButton.onClick = [this] { undoManager.redo(); };
    saveButton.onClick = [this] { launchPresetChooser (true); };
    loadButton.onClick = [this] { launchPresetChooser (false); };

    // Link-Transport: Slider schreibt in die Session, der Timer pollt zurück
    tempoSlider.setRange (20.0, 300.0, 0.1);
    tempoSlider.setTextValueSuffix (" BPM");
    tempoSlider.setValue (linkClock.getTempo(), juce::dontSendNotification);
    tempoSlider.onValueChange = [this] { linkClock.setTempo (tempoSlider.getValue()); };

    peersLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
    peersLabel.setJustificationType (juce::Justification::centredLeft);

    warningLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
    warningLabel.setJustificationType (juce::Justification::centredRight);

    const auto warning = rootState.getProperty (id::audioSetupWarning).toString();

    if (warning.isNotEmpty())
        warningLabel.setText ("Audio-Setup: " + warning, juce::dontSendNotification);

    addAndMakeVisible (addButton);
    addAndMakeVisible (addLfoButton);
    addAndMakeVisible (addScopeButton);
    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (tempoSlider);
    addAndMakeVisible (peersLabel);
    addAndMakeVisible (warningLabel);
    addAndMakeVisible (canvas);

    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setSize (1180, 680);

    timerCallback();    // Peer-Label sofort befüllen, nicht erst nach 250ms
    startTimerHz (4);   // Session-Polling — Tempo/Peers können sich im Netz ändern
}

//==============================================================================
void EngineEditor::launchPresetChooser (bool saving)
{
    const auto defaultDirectory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                      .getChildFile ("Conduit");
    defaultDirectory.createDirectory();

    // Async — Modal-Loops sind projektweit abgeschaltet (13.2)
    presetChooser = std::make_unique<juce::FileChooser> (
        saving ? "Preset speichern" : "Preset laden",
        defaultDirectory,
        "*" + juce::String (EngineProcessor::presetFileExtension));

    // 'chooserFlags' — 'flags' würde juce::Component::flags verschatten (C4458)
    const auto chooserFlags = saving
        ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
              | juce::FileBrowserComponent::warnAboutOverwriting
        : juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    presetChooser->launchAsync (chooserFlags, [this, saving] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();

        if (file == juce::File())
            return;  // abgebrochen

        if (saving)
            file = file.withFileExtension (EngineProcessor::presetFileExtension);

        const auto result = saving ? engine.savePreset (file)
                                   : engine.loadPreset (file);

        if (result.failed())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    saving ? "Preset speichern" : "Preset laden",
                                                    result.getErrorMessage());
    });
}

//==============================================================================
void EngineEditor::timerCallback()
{
    // Kein Kampf mit dem User: während des Drags gewinnt der Slider
    if (! tempoSlider.isMouseButtonDown())
        tempoSlider.setValue (linkClock.getTempo(), juce::dontSendNotification);

    const auto numPeers = linkClock.getNumPeers();
    const auto text = numPeers == 0
                          ? juce::String ("Link: keine Peers")
                          : "Link: " + juce::String (numPeers)
                                + (numPeers == 1 ? " Peer" : " Peers");

    if (peersLabel.getText() != text)
        peersLabel.setText (text, juce::dontSendNotification);
}

//==============================================================================
void EngineEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff24272c));  // Toolbar-Hintergrund
}

void EngineEditor::resized()
{
    auto bounds = getLocalBounds();
    auto toolbar = bounds.removeFromTop (toolbarHeight).reduced (8, 6);  // Buttons ≥ 44px hoch

    const auto place = [&toolbar] (juce::Component& component, int width, int gapAfter = 8)
    {
        component.setBounds (toolbar.removeFromLeft (width));
        toolbar.removeFromLeft (gapAfter);
    };

    place (addButton,      110);
    place (addLfoButton,    90);
    place (addScopeButton, 100);
    place (undoButton,      70);
    place (redoButton,      70, 16);
    place (saveButton,      70);
    place (loadButton,      70, 16);
    place (tempoSlider,    140);
    place (peersLabel,     130);
    warningLabel.setBounds (toolbar);

    canvas.setBounds (bounds);
}

bool EngineEditor::keyPressed (const juce::KeyPress& key)
{
    const auto modifier = juce::ModifierKeys::commandModifier;

    if (key == juce::KeyPress ('z', modifier, 0))
        return undoManager.undo();

    if (key == juce::KeyPress ('y', modifier, 0)
        || key == juce::KeyPress ('z', modifier | juce::ModifierKeys::shiftModifier, 0))
        return undoManager.redo();

    return false;
}

} // namespace conduit
