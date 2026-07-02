#include <memory>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/GraphFader.h"
#include "Core/GraphManager.h"
#include "Core/NodeUiRegistry.h"
#include "Core/OscAddress.h"
#include "Core/OscController.h"
#include "Core/OscSendService.h"
#include "Core/OscSendSettings.h"
#include "Modules/ModuleFactory.h"

using Catch::Approx;

namespace
{

//==============================================================================
/** Persistenz in ein Temp-Verzeichnis statt in die echte Settings-Datei. */
struct TempOscSendSettings
{
    TempOscSendSettings()
        : folder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("ConduitOscSendSettingsTests")
                      .getChildFile (juce::Uuid().toString()))
    {
        folder.createDirectory();
    }

    ~TempOscSendSettings() { folder.deleteRecursively(); }

    [[nodiscard]] juce::PropertiesFile::Options options() const
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "ConduitOscSendSettingsTests";
        o.filenameSuffix  = ".settings";
        o.folderName      = folder.getFullPathName();  // absoluter Pfad
        return o;
    }

    juce::File folder;
};

//==============================================================================
/** Fängt Bundles statt UDP — der IOscSink-Seam des OscSendService. */
struct FakeSink final : conduit::IOscSink
{
    bool connect (const juce::String& h, int p) override
    {
        host = h;
        port = p;
        ++connectCalls;
        return connectResult;
    }

    void disconnect() override { ++disconnectCalls; }

    bool sendBundle (const juce::OSCBundle& bundle) override
    {
        bundles.push_back (bundle);
        return true;
    }

    /** Alle Messages aller Bundles, flach. */
    [[nodiscard]] std::vector<juce::OSCMessage> messages() const
    {
        std::vector<juce::OSCMessage> flat;

        for (const auto& bundle : bundles)
            for (int e = 0; e < bundle.size(); ++e)
                if (bundle[e].isMessage())
                    flat.push_back (bundle[e].getMessage());

        return flat;
    }

    void clear() { bundles.clear(); }

    juce::String host;
    int port = 0;
    int connectCalls = 0;
    int disconnectCalls = 0;
    bool connectResult = true;
    std::vector<juce::OSCBundle> bundles;
};

//==============================================================================
juce::ValueTree makeRootTree()
{
    juce::ValueTree root (conduit::id::root);
    root.appendChild (juce::ValueTree (conduit::id::nodes),               nullptr);
    root.appendChild (juce::ValueTree (conduit::id::connections),         nullptr);
    root.appendChild (juce::ValueTree (conduit::id::calibrationProfiles), nullptr);
    return root;
}

/** Gemeinsames Setup: Service mit Fake-Sink an einem Tree mit Modul-States
    (der Send-Pfad braucht keinen GraphManager — reiner Tree-Walk). */
struct SendTestRig
{
    SendTestRig()
        : settings (temp.options())
    {
        conduit::registerDefaultModules (factory);

        auto sinkOwned = std::make_unique<FakeSink>();
        sink = sinkOwned.get();
        service = std::make_unique<conduit::OscSendService> (root, settings,
                                                             std::move (sinkOwned));
    }

    [[nodiscard]] juce::ValueTree nodes() { return root.getChildWithName (conduit::id::nodes); }

    juce::ValueTree addModuleNodeWithState (const juce::String& factoryKey)
    {
        auto module = factory.create (factoryKey);
        REQUIRE (module != nullptr);

        auto node = module->createState();
        nodes().appendChild (node, nullptr);
        return node;
    }

    static juce::ValueTree parameterOf (juce::ValueTree node, const juce::String& paramId)
    {
        return node.getChildWithName (conduit::id::parameters)
                   .getChildWithProperty (conduit::id::paramId, paramId);
    }

    void enable()
    {
        settings.setEnabled (true);
        settings.dispatchPendingMessages();  // synchrone Zustellung
    }

    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempOscSendSettings temp;
    juce::ValueTree root = makeRootTree();
    conduit::ModuleFactory factory;
    conduit::OscSendSettings settings;
    FakeSink* sink = nullptr;
    std::unique_ptr<conduit::OscSendService> service;
};

} // namespace

//==============================================================================
TEST_CASE ("OscSendSettings: Defaults und Persistenz-Roundtrip", "[oscsend][io]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempOscSendSettings temp;

    {
        conduit::OscSendSettings settings (temp.options());
        REQUIRE (settings.getHost() == juce::String (conduit::OscSendSettings::defaultHost));
        REQUIRE (settings.getPort() == conduit::OscSendSettings::defaultPort);
        REQUIRE_FALSE (settings.isEnabled());

        settings.setHost ("192.168.1.20");
        settings.setPort (8000);
        settings.setEnabled (true);
    }

    conduit::OscSendSettings reloaded (temp.options());
    REQUIRE (reloaded.getHost() == "192.168.1.20");
    REQUIRE (reloaded.getPort() == 8000);
    REQUIRE (reloaded.isEnabled());
}

TEST_CASE ("OscSendSettings: ungültige Werte werden verworfen", "[oscsend]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    TempOscSendSettings temp;
    conduit::OscSendSettings settings (temp.options());

    settings.setHost ("  ");
    settings.setPort (0);
    settings.setPort (70000);

    REQUIRE (settings.getHost() == juce::String (conduit::OscSendSettings::defaultHost));
    REQUIRE (settings.getPort() == conduit::OscSendSettings::defaultPort);
}

//==============================================================================
TEST_CASE ("OscSendService: disabled sendet nie", "[oscsend]")
{
    SendTestRig rig;
    rig.addModuleNodeWithState ("attenuator");

    rig.service->flushPendingSend();
    rig.service->sendFullDump();

    REQUIRE (rig.sink->bundles.empty());
    REQUIRE (rig.sink->connectCalls == 0);
}

TEST_CASE ("OscSendService: Aktivierung verbindet und der erste Tick ist ein Voll-Sync", "[oscsend]")
{
    SendTestRig rig;
    auto node = rig.addModuleNodeWithState ("attenuator");

    rig.enable();
    REQUIRE (rig.sink->connectCalls == 1);
    REQUIRE (rig.sink->host == juce::String (conduit::OscSendSettings::defaultHost));
    REQUIRE (rig.sink->port == conduit::OscSendSettings::defaultPort);
    REQUIRE (rig.service->isConnected());

    rig.service->flushPendingSend();

    const auto messages = rig.sink->messages();
    REQUIRE (messages.size() == 1);  // Attenuator hat genau "gain"
    REQUIRE (messages[0].getAddressPattern().toString() == "/conduit/utility/attenuator/gain");
    REQUIRE (messages[0][0].isFloat32());
    REQUIRE (messages[0][0].getFloat32()
             == Approx ((float) (double) SendTestRig::parameterOf (node, "gain")
                            .getProperty (conduit::id::paramValue)));
}

TEST_CASE ("OscSendService: nur Änderungen werden gesendet (Snapshot-Diff)", "[oscsend]")
{
    SendTestRig rig;
    auto node = rig.addModuleNodeWithState ("attenuator");

    rig.enable();
    rig.service->flushPendingSend();  // Voll-Sync
    rig.sink->clear();

    // Unverändert → kein Send
    rig.service->flushPendingSend();
    REQUIRE (rig.sink->messages().empty());

    // UI-Write → genau eine Message mit dem neuen Wert
    SendTestRig::parameterOf (node, "gain")
        .setProperty (conduit::id::paramValue, 0.25f, nullptr);
    rig.service->flushPendingSend();

    auto messages = rig.sink->messages();
    REQUIRE (messages.size() == 1);
    REQUIRE (messages[0].getAddressPattern().toString() == "/conduit/utility/attenuator/gain");
    REQUIRE (messages[0][0].getFloat32() == Approx (0.25f));

    // Derselbe Wert nochmal getickt → nichts (double/float-Roundtrip stabil)
    rig.sink->clear();
    rig.service->flushPendingSend();
    rig.service->flushPendingSend();
    REQUIRE (rig.sink->messages().empty());
}

TEST_CASE ("OscSendService: noteRemoteValue unterdrückt das Echo, UI-Write danach sendet wieder", "[oscsend]")
{
    SendTestRig rig;
    auto node = rig.addModuleNodeWithState ("attenuator");

    rig.enable();
    rig.service->flushPendingSend();
    rig.sink->clear();

    // OSC-Empfang: Tree-Write + Cache-Impfung (Reihenfolge wie
    // OscController::applyTreeUpdates) → nächster Tick sieht keinen Diff
    const auto nodeUuid = node.getProperty (conduit::id::nodeId).toString();
    SendTestRig::parameterOf (node, "gain")
        .setProperty (conduit::id::paramValue, 0.7f, nullptr);
    rig.service->noteRemoteValue (nodeUuid, "gain", 0.7f);

    rig.service->flushPendingSend();
    REQUIRE (rig.sink->messages().empty());

    // Anschließender UI-Write geht wieder raus
    SendTestRig::parameterOf (node, "gain")
        .setProperty (conduit::id::paramValue, 0.9f, nullptr);
    rig.service->flushPendingSend();

    const auto messages = rig.sink->messages();
    REQUIRE (messages.size() == 1);
    REQUIRE (messages[0][0].getFloat32() == Approx (0.9f));
}

TEST_CASE ("OscSendService: Deleting-Nodes werden übersprungen (5.3 Phase 1)", "[oscsend]")
{
    SendTestRig rig;
    auto node = rig.addModuleNodeWithState ("attenuator");

    rig.enable();
    node.setProperty (conduit::id::nodeState,
                      conduit::toString (conduit::NodeState::deleting), nullptr);

    rig.service->flushPendingSend();
    rig.service->sendFullDump();

    REQUIRE (rig.sink->messages().empty());
}

TEST_CASE ("OscSendService: sendFullDump sendet alles unabhängig vom Cache", "[oscsend]")
{
    SendTestRig rig;
    rig.addModuleNodeWithState ("attenuator");
    rig.addModuleNodeWithState ("lfo");  // rate + depth

    rig.enable();
    rig.service->flushPendingSend();  // Cache warm
    rig.sink->clear();

    rig.service->sendFullDump();

    REQUIRE (rig.sink->messages().size() == 3);  // gain + rate + depth
}

TEST_CASE ("OscSendService: Chunking bei mehr als maxMessagesPerBundle Messages", "[oscsend]")
{
    SendTestRig rig;

    const auto perNode = 1;  // Attenuator: gain
    const auto nodesNeeded = conduit::OscSendService::maxMessagesPerBundle / perNode + 10;

    for (int i = 0; i < nodesNeeded; ++i)
    {
        auto node = rig.addModuleNodeWithState ("attenuator");
        node.setProperty (conduit::id::moduleId, "attenuator_" + juce::String (i), nullptr);
    }

    rig.enable();
    rig.service->flushPendingSend();

    REQUIRE (rig.sink->bundles.size() == 2);
    REQUIRE ((int) rig.sink->messages().size() == nodesNeeded);
    REQUIRE (rig.sink->bundles.front().size()
             == conduit::OscSendService::maxMessagesPerBundle);
}

TEST_CASE ("OscSendService: Cache-Pruning — entfernter Node sendet nach Wiederkehr erneut", "[oscsend]")
{
    SendTestRig rig;
    auto node = rig.addModuleNodeWithState ("attenuator");

    rig.enable();
    rig.service->flushPendingSend();
    rig.sink->clear();

    rig.nodes().removeChild (node, nullptr);
    rig.service->flushPendingSend();  // pruned den Cache-Eintrag
    REQUIRE (rig.sink->messages().empty());

    rig.nodes().appendChild (node, nullptr);
    rig.service->flushPendingSend();  // wieder da → Voll-Sync für den Node

    REQUIRE (rig.sink->messages().size() == 1);
}

TEST_CASE ("OscSendService: Deaktivieren trennt und stoppt", "[oscsend]")
{
    SendTestRig rig;
    rig.addModuleNodeWithState ("attenuator");

    rig.enable();
    REQUIRE (rig.service->isConnected());

    rig.settings.setEnabled (false);
    rig.settings.dispatchPendingMessages();

    REQUIRE_FALSE (rig.service->isConnected());
    rig.service->flushPendingSend();
    REQUIRE (rig.sink->messages().empty());
}

//==============================================================================
TEST_CASE ("OscAddress: Send-Adressen == Receive-Registry-Adressen", "[oscsend][osc]")
{
    // Voller Receive-Stack (GraphManager + OscController) gegen den Helper
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    juce::ValueTree root = makeRootTree();
    juce::AudioProcessorGraph graph;
    conduit::GraphFader fader;
    conduit::ModuleFactory factory;
    juce::UndoManager undoManager;
    conduit::NodeUiRegistry uiRegistry;
    conduit::GraphManager manager { root, graph, fader, factory, undoManager, uiRegistry };
    conduit::SpscQueue<conduit::ParameterUpdate> audioQueue { 64 };
    conduit::OscController osc { root, manager, audioQueue };

    conduit::registerDefaultModules (factory);

    auto module = factory.create ("attenuator");
    REQUIRE (module != nullptr);
    auto node = module->createState();
    root.getChildWithName (conduit::id::nodes).appendChild (node, nullptr);

    manager.flushPendingTopologyUpdate();
    osc.flushPendingUpdates();

    const auto registered = osc.getRegisteredAddresses();
    REQUIRE (registered.contains (conduit::osc::parameterAddress (node, "gain")));
    REQUIRE (conduit::osc::parameterAddress (node, "gain")
             == "/conduit/utility/attenuator/gain");
}
