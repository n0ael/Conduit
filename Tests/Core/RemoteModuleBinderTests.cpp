#include <atomic>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "Core/GraphFader.h"
#include "Core/GraphManager.h"
#include "Core/NodeUiRegistry.h"
#include "Core/OscAddress.h"
#include "Core/OscController.h"
#include "Core/RemoteModuleBinder.h"
#include "Modules/LfoModule.h"
#include "Modules/ModuleFactory.h"

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

/** Voller Stack: GraphManager + OscController + Binder — Verdrahtung wie
    im EngineProcessor (onAnnounce → handleAnnounce). */
struct BinderRig
{
    explicit BinderRig (juce::ValueTree existingRoot = {})
        : root (existingRoot.isValid() ? existingRoot : makeRootTree())
    {
        conduit::registerDefaultModules (factory);
        osc.onAnnounce = [this] (const conduit::osc::AnnounceInfo& info)
        {
            lastBound = binder.handleAnnounce (info);
        };
    }

    [[nodiscard]] juce::ValueTree nodes() { return root.getChildWithName (conduit::id::nodes); }

    void flushAll()
    {
        manager.flushPendingTopologyUpdate();
        osc.flushPendingUpdates();
    }

    /** Announce über den echten Empfangspfad (Netzwerk-Parsing inklusive). */
    void announce (const juce::String& remoteId, const juce::String& factoryKey,
                   const juce::String& trackName, int colour)
    {
        juce::OSCMessage message { juce::OSCAddressPattern (conduit::osc::announceAddress) };
        message.addString (remoteId);
        message.addString (factoryKey);
        message.addString (trackName);
        message.addInt32 (colour);
        osc.oscMessageReceived (message);
        flushAll();
    }

    juce::ScopedJuceInitialiser_GUI juceRuntime;
    juce::ValueTree root;
    juce::AudioProcessorGraph graph;
    conduit::GraphFader fader;
    conduit::ModuleFactory factory;
    juce::UndoManager undoManager;
    conduit::NodeUiRegistry uiRegistry;
    conduit::GraphManager manager { root, graph, fader, factory, undoManager, uiRegistry };
    conduit::SpscQueue<conduit::ParameterUpdate> audioQueue { 256 };
    conduit::OscController osc { root, manager, audioQueue };
    conduit::RemoteModuleBinder binder { root, manager, factory };
    juce::ValueTree lastBound;
};

} // namespace

//==============================================================================
TEST_CASE ("Announce legt den gespiegelten Node an (factoryId/remoteId/Tint/Name)", "[announce]")
{
    BinderRig rig;
    rig.announce ("liveDev-1", "lfo", "My Track", 0x00ff8800);

    REQUIRE (rig.nodes().getNumChildren() == 1);
    const auto node = rig.nodes().getChild (0);

    REQUIRE (node.getProperty (conduit::id::factoryId).toString() == "lfo");
    REQUIRE (node.getProperty (conduit::id::remoteId).toString() == "liveDev-1");
    REQUIRE ((int) node.getProperty (conduit::id::tintColour) == 0x00ff8800);
    REQUIRE (node.getProperty (conduit::id::moduleId).toString() == "my_track");
    REQUIRE (rig.lastBound == node);
}

TEST_CASE ("Re-Announce ist idempotent — nur der Tint zieht nach", "[announce]")
{
    BinderRig rig;
    rig.announce ("dev", "lfo", "Pad", 0x00112233);

    auto node = rig.nodes().getChild (0);
    node.setProperty (conduit::id::moduleId, "user_renamed", nullptr);  // User-Hoheit

    rig.announce ("dev", "lfo", "Pad (neu)", 0x00445566);

    REQUIRE (rig.nodes().getNumChildren() == 1);  // kein Duplikat
    REQUIRE (node.getProperty (conduit::id::moduleId).toString() == "user_renamed");
    REQUIRE ((int) node.getProperty (conduit::id::tintColour) == 0x00445566);
}

TEST_CASE ("Namens-Kollision: Auto-Name bleibt, der Node entsteht trotzdem", "[announce]")
{
    BinderRig rig;
    rig.announce ("dev-a", "lfo", "Bass", 0);
    rig.announce ("dev-b", "lfo", "Bass", 0);  // gleicher Wunsch-Name

    REQUIRE (rig.nodes().getNumChildren() == 2);
    const auto second = rig.nodes().getChild (1);
    REQUIRE (second.getProperty (conduit::id::remoteId).toString() == "dev-b");
    REQUIRE (second.getProperty (conduit::id::moduleId).toString() != "bass");  // Auto-Name
}

TEST_CASE ("Garbage-Announces werden still verworfen", "[announce]")
{
    BinderRig rig;

    // Unbekannter factoryKey (Whitelist)
    rig.announce ("dev", "not_a_module", "X", 0);
    REQUIRE (rig.nodes().getNumChildren() == 0);

    // Ungültige remoteId (Zeichen/Länge — wird Teil von OSC-Adressen)
    rig.announce ("bad id/with#stuff", "lfo", "X", 0);
    rig.announce (juce::String::repeatedString ("x", 65), "lfo", "X", 0);
    REQUIRE (rig.nodes().getNumChildren() == 0);

    // Fehlende/falsch typisierte Argumente über den Empfangspfad
    juce::OSCMessage tooFew { juce::OSCAddressPattern (conduit::osc::announceAddress) };
    tooFew.addString ("dev");
    rig.osc.oscMessageReceived (tooFew);

    juce::OSCMessage wrongTypes { juce::OSCAddressPattern (conduit::osc::announceAddress) };
    wrongTypes.addInt32 (1);
    wrongTypes.addInt32 (2);
    wrongTypes.addInt32 (3);
    wrongTypes.addInt32 (4);
    rig.osc.oscMessageReceived (wrongTypes);

    rig.flushAll();
    REQUIRE (rig.nodes().getNumChildren() == 0);
}

TEST_CASE ("Alias-Adresse erreicht dasselbe Atomic wie die kanonische", "[announce][osc]")
{
    BinderRig rig;
    rig.announce ("dev", "lfo", "Wobble", 0);
    rig.flushAll();  // zweiter Durchlauf: Modul materialisiert, Targets aufgelöst

    const auto node = rig.nodes().getChild (0);
    auto* module = rig.manager.getModuleFor (node.getProperty (conduit::id::nodeId).toString());
    REQUIRE (module != nullptr);
    auto* rateTarget = module->getParameterTarget ("rate");
    REQUIRE (rateTarget != nullptr);

    const auto sendTo = [&rig] (const juce::String& address, float value)
    {
        juce::OSCMessage message { juce::OSCAddressPattern (address) };
        message.addFloat32 (value);
        rig.osc.oscMessageReceived (message);
    };

    // Alias-Pfad
    sendTo (conduit::osc::remoteAliasAddress ("dev", "rate"), 2.0f);
    conduit::ParameterUpdate update;
    REQUIRE (rig.audioQueue.pop (update));
    REQUIRE (update.target == rateTarget);

    // Kanonischer Pfad trifft dasselbe Target
    sendTo (conduit::osc::parameterAddress (node, "rate"), 1.0f);
    REQUIRE (rig.audioQueue.pop (update));
    REQUIRE (update.target == rateTarget);

    rig.flushAll();
}

TEST_CASE ("Rename: kanonische Adresse wandert, der Alias bleibt", "[announce][osc]")
{
    BinderRig rig;
    rig.announce ("dev", "lfo", "Alt", 0);
    rig.flushAll();

    const auto node = rig.nodes().getChild (0);
    const auto uuid = node.getProperty (conduit::id::nodeId).toString();
    const auto oldCanonical = conduit::osc::parameterAddress (node, "rate");

    REQUIRE (rig.manager.renameNode (uuid, "neuer_name"));
    rig.flushAll();

    const auto registered = rig.osc.getRegisteredAddresses();
    REQUIRE (registered.contains (conduit::osc::remoteAliasAddress ("dev", "rate")));
    REQUIRE (registered.contains (conduit::osc::parameterAddress (node, "rate")));
    REQUIRE_FALSE (registered.contains (oldCanonical));
}

TEST_CASE ("Save/Load/Re-Announce: remoteId übersteht die Serialisierung, kein Duplikat", "[announce]")
{
    juce::ValueTree reloaded;

    {
        BinderRig rig;
        rig.announce ("persist-1", "lfo", "Keys", 0x00123456);

        const auto xml = rig.root.createXml();
        REQUIRE (xml != nullptr);
        reloaded = juce::ValueTree::fromXml (*xml);
    }

    BinderRig rig (reloaded);
    rig.flushAll();  // Graph aus dem geladenen Tree aufbauen
    REQUIRE (rig.nodes().getNumChildren() == 1);

    rig.announce ("persist-1", "lfo", "Keys", 0x00654321);

    REQUIRE (rig.nodes().getNumChildren() == 1);  // gefunden statt neu angelegt
    REQUIRE ((int) rig.nodes().getChild (0).getProperty (conduit::id::tintColour)
             == 0x00654321);
}

//==============================================================================
TEST_CASE ("Announce-Dauerfeuer nebenläufig zum Registry-Rebuild", "[announce][osc][threading]")
{
    BinderRig rig;

    std::atomic<bool> networkDone { false };

    // Netzwerk-Thread: gültige und Garbage-Announces im Wechsel
    std::thread networkThread ([&rig, &networkDone]
    {
        for (int i = 0; i < 300; ++i)
        {
            juce::OSCMessage valid { juce::OSCAddressPattern (conduit::osc::announceAddress) };
            valid.addString ("stress-" + juce::String (i % 5));
            valid.addString ("lfo");
            valid.addString ("Track " + juce::String (i));
            valid.addInt32 (i);
            rig.osc.oscMessageReceived (valid);

            juce::OSCMessage garbage { juce::OSCAddressPattern (conduit::osc::announceAddress) };
            garbage.addString ("invalid/#");
            garbage.addString ("not_a_module");
            garbage.addString ("x");
            garbage.addInt32 (0);
            rig.osc.oscMessageReceived (garbage);
        }

        networkDone.store (true, std::memory_order_release);
    });

    // Message Thread: Announces konsumieren + Registry-Rebuild erzwingen
    while (! networkDone.load (std::memory_order_acquire))
    {
        rig.flushAll();
        std::this_thread::yield();
    }

    networkThread.join();
    rig.flushAll();
    rig.flushAll();  // Materialisierung nachziehen

    // Genau 5 remoteIds → genau 5 Nodes, Garbage hat nichts angelegt
    REQUIRE (rig.nodes().getNumChildren() == 5);
}
