#include "EngineEditor.h"

#include "EngineProcessor.h"
#include "Modules/AttenuatorModule.h"
#include "Modules/LfoModule.h"
#include "Modules/ScopeModule.h"
#include "Modules/StepSequencerModule.h"
#include "Util/ScaleQuantizer.h"

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
    addSeqButton.onClick   = [addModule] { addModule (StepSequencerModule::staticModuleId); };

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

    // Globale Session-Skala (Schema 6.2): schreibt die Root-Properties —
    // bewusst ohne UndoManager (Session-Setting wie Parameter-Sweeps);
    // preset-persistent ist sie über den Tree trotzdem
    {
        const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F",
                                    "F#", "G", "G#", "A", "A#", "B" };

        for (int note = 0; note < 12; ++note)
            rootCombo.addItem (noteNames[note], note + 1);

        for (const auto type : { ScaleType::chromatic, ScaleType::major,
                                 ScaleType::minor, ScaleType::pentatonic })
            scaleCombo.addItem (toString (type), static_cast<int> (type) + 1);

        rootCombo.setSelectedId (juce::jlimit (0, 11,
            (int) rootState.getProperty (id::scaleRoot, 0)) + 1, juce::dontSendNotification);
        scaleCombo.setSelectedId (static_cast<int> (scaleTypeFromString (
            rootState.getProperty (id::scaleType).toString())) + 1, juce::dontSendNotification);

        rootCombo.onChange = [this]
        { rootState.setProperty (id::scaleRoot, rootCombo.getSelectedId() - 1, nullptr); };
        scaleCombo.onChange = [this]
        {
            rootState.setProperty (id::scaleType,
                toString (static_cast<ScaleType> (scaleCombo.getSelectedId() - 1)), nullptr);
        };
    }

    // OSC-Status (verbunden in Main::initialise)
    const auto oscPort = engine.getOscController().getConnectedPort();
    oscLabel.setText (oscPort > 0 ? "OSC :" + juce::String (oscPort)
                                  : juce::String ("OSC: aus"),
                      juce::dontSendNotification);
    oscLabel.setColour (juce::Label::textColourId,
                        oscPort > 0 ? juce::Colours::white.withAlpha (0.7f)
                                    : juce::Colours::orange);
    oscLabel.setJustificationType (juce::Justification::centredLeft);

    warningLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
    warningLabel.setJustificationType (juce::Justification::centredRight);

    const auto warning = rootState.getProperty (id::audioSetupWarning).toString();

    if (warning.isNotEmpty())
        warningLabel.setText ("Audio-Setup: " + warning, juce::dontSendNotification);

    addAndMakeVisible (addButton);
    addAndMakeVisible (addLfoButton);
    addAndMakeVisible (addScopeButton);
    addAndMakeVisible (addSeqButton);
    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (tempoSlider);
    addAndMakeVisible (rootCombo);
    addAndMakeVisible (scaleCombo);
    addAndMakeVisible (peersLabel);
    addAndMakeVisible (oscLabel);
    addAndMakeVisible (warningLabel);
    addAndMakeVisible (canvas);

    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setSize (1320, 720);

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

    place (addButton,       95);
    place (addLfoButton,    80);
    place (addScopeButton,  90);
    place (addSeqButton,    80);
    place (undoButton,      65);
    place (redoButton,      65, 16);
    place (saveButton,      65);
    place (loadButton,      65, 16);
    place (tempoSlider,    130);
    place (rootCombo,       60);
    place (scaleCombo,     110, 16);
    place (peersLabel,     115);
    place (oscLabel,        80);
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
