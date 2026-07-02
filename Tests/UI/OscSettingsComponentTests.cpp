#include <catch2/catch_test_macros.hpp>

#include "Core/GraphFader.h"
#include "Core/GraphManager.h"
#include "Core/NodeUiRegistry.h"
#include "Core/OscController.h"
#include "Core/OscSendSettings.h"
#include "Modules/ModuleFactory.h"
#include "UI/OscSettingsComponent.h"

namespace
{

juce::ValueTree makeRootTree()
{
    juce::ValueTree root (conduit::id::root);
    root.appendChild (juce::ValueTree (conduit::id::nodes),               nullptr);
    root.appendChild (juce::ValueTree (conduit::id::connections),         nullptr);
    root.appendChild (juce::ValueTree (conduit::id::calibrationProfiles), nullptr);
    return root;
}

/** Persistenz in ein Temp-Verzeichnis statt in die echte Settings-Datei. */
struct TempSettingsFolder
{
    TempSettingsFolder()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitOscSettingsComponentTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempSettingsFolder() { folder.deleteRecursively(); }

    [[nodiscard]] juce::PropertiesFile::Options options() const
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "ConduitOscSettingsComponentTests";
        o.filenameSuffix  = ".settings";
        o.folderName      = folder.getFullPathName();
        return o;
    }

    juce::File folder;
};

/** Minimaler Controller-Rig (die Komponente zeigt nur den Empfangsport). */
struct ComponentRig
{
    ComponentRig() : settings (temp.options()) {}

    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempSettingsFolder temp;
    juce::ValueTree root = makeRootTree();
    juce::AudioProcessorGraph graph;
    conduit::GraphFader fader;
    conduit::ModuleFactory factory;
    juce::UndoManager undoManager;
    conduit::NodeUiRegistry uiRegistry;
    conduit::GraphManager manager { root, graph, fader, factory, undoManager, uiRegistry };
    conduit::SpscQueue<conduit::ParameterUpdate> audioQueue { 64 };
    conduit::OscController osc { root, manager, audioQueue };
    conduit::OscSendSettings settings;
};

} // namespace

//==============================================================================
TEST_CASE ("OscSettingsComponent: Controls zeigen die Settings-Defaults", "[oscsend][ui]")
{
    ComponentRig rig;
    conduit::OscSettingsComponent component (rig.settings, rig.osc);

    REQUIRE (component.hostEditor.getText()
             == juce::String (conduit::OscSendSettings::defaultHost));
    REQUIRE (component.portEditor.getText().getIntValue()
             == conduit::OscSendSettings::defaultPort);
    REQUIRE_FALSE (component.enableToggle.getToggleState());

    component.setSize (520, 480);  // Layout crasht nicht headless
}

TEST_CASE ("OscSettingsComponent: Edits schreiben in die Settings", "[oscsend][ui]")
{
    ComponentRig rig;
    conduit::OscSettingsComponent component (rig.settings, rig.osc);

    component.hostEditor.setText ("10.0.0.7", juce::dontSendNotification);
    component.applyHostEdit();
    REQUIRE (rig.settings.getHost() == "10.0.0.7");

    component.portEditor.setText ("9500", juce::dontSendNotification);
    component.applyPortEdit();
    REQUIRE (rig.settings.getPort() == 9500);

    component.enableToggle.setToggleState (true, juce::dontSendNotification);
    component.enableToggle.onClick();
    REQUIRE (rig.settings.isEnabled());

    // Ungültige Eingabe wird verworfen und zurückgedreht
    component.hostEditor.setText ("   ", juce::dontSendNotification);
    component.applyHostEdit();
    REQUIRE (rig.settings.getHost() == "10.0.0.7");
    REQUIRE (component.hostEditor.getText() == "10.0.0.7");
}

TEST_CASE ("OscSettingsComponent: Settings-Broadcast zieht die Controls nach", "[oscsend][ui]")
{
    ComponentRig rig;
    conduit::OscSettingsComponent component (rig.settings, rig.osc);

    rig.settings.setHost ("192.168.0.42");
    rig.settings.setPort (7001);
    rig.settings.dispatchPendingMessages();

    REQUIRE (component.hostEditor.getText() == "192.168.0.42");
    REQUIRE (component.portEditor.getText() == "7001");
}
