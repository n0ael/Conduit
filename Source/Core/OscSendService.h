#pragma once

#include <map>
#include <memory>
#include <utility>

#include <juce_data_structures/juce_data_structures.h>
#include <juce_osc/juce_osc.h>

#include "OscSendSettings.h"

namespace conduit
{

//==============================================================================
/** Transport-Seam des OscSendService — Tests injizieren einen Fake-Sink,
    die App den juce::OSCSender (makeUdpOscSink). */
class IOscSink
{
public:
    virtual ~IOscSink() = default;

    virtual bool connect (const juce::String& host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool sendBundle (const juce::OSCBundle& bundle) = 0;
};

/** UDP-Implementierung via juce::OSCSender. */
[[nodiscard]] std::unique_ptr<IOscSink> makeUdpOscSink();

//==============================================================================
/**
    OSC-Send-Pfad (CLAUDE.md 7.3): Parameter-Feedback an Clients
    (TouchOSC-Fader folgen Conduit) als Snapshot-Diff-Timer.

    Kein paramValue-Listener — der kann OSC-Empfang, UI, Undo und Preset-Load
    nicht unterscheiden. Stattdessen 30-Hz-Tree-Walk über Nodes[] mit
    Vergleich gegen den lastSent-Cache, Key (nodeUuid, paramId) — bewusst
    NICHT die Adresse, damit moduleId-Renames keine Geister-Diffs erzeugen.

    Echo-Suppression: der OscController ruft nach jedem in den Tree
    übernommenen Empfangswert noteRemoteValue() (via onRemoteValueApplied,
    beide Message Thread) und impft den Cache VOR dem nächsten Tick — der
    eigene Empfang wird nie zurückgesendet. UI-/Undo-/Preset-Load-Writes
    diffen dagegen und gehen raus (gewollt).

    Aktivierung leert den Cache → der erste Tick ist ein impliziter
    Voll-Sync. Pro Tick ein OSCBundle, Chunking bei > maxMessagesPerBundle
    Messages (UDP-Paketgrenze). Deleting-Nodes (5.3 Phase 1) werden wie in
    der Receive-Registry übersprungen; Cache-Einträge verschwundener Nodes
    werden gepruned.

    Läuft vollständig auf dem Message Thread — der Audio Thread ist NIE
    beteiligt (3.1 bleibt gewahrt), kein Lock.
*/
class OscSendService final : private juce::Timer,
                             private juce::ChangeListener
{
public:
    static constexpr int ticksPerSecond = 30;
    static constexpr int maxMessagesPerBundle = 50;

    /** sink nullptr → juce::OSCSender (App-Pfad); Tests injizieren einen
        Fake. Lauscht auf die Settings und verbindet sofort, falls enabled. */
    OscSendService (juce::ValueTree rootTree,
                    OscSendSettings& settingsToUse,
                    std::unique_ptr<IOscSink> sinkToUse = nullptr);

    ~OscSendService() override;

    //==========================================================================
    // Message Thread

    /** Echo-Impfung (7.3): merkt einen via OSC empfangenen, bereits in den
        Tree übernommenen Wert als gesendet — der nächste Tick diffed ihn
        nicht mehr. Verdrahtet über OscController::onRemoteValueApplied. */
    void noteRemoteValue (const juce::String& nodeUuid,
                          const juce::String& parameterId,
                          float value);

    /** Voll-Dump aller registrierten Parameter (Antwort auf /conduit/sync) —
        sendet unabhängig vom Cache und aktualisiert ihn. No-op wenn disabled. */
    void sendFullDump();

    /** Voll-Dump eines einzelnen Nodes (Re-Announce, 7.4). */
    void sendNodeValues (const juce::String& nodeUuid);

    /** Test-Seam: führt den nächsten Timer-Tick sofort synchron aus. */
    void flushPendingSend();

    /** true, wenn enabled UND der Sink verbunden ist — für die Status-UI. */
    [[nodiscard]] bool isConnected() const noexcept { return connected; }

private:
    //==========================================================================
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    /** Settings → Sink: (re)connect/disconnect + Timer-Start/-Stopp.
        Jede Änderung leert den Cache (Neuziel = frischer Voll-Sync). */
    void applySettings();

    /** Ein Tick: Tree-Walk, Diff (force → alles), Bundle(s) senden.
        onlyNodeUuid nicht leer → nur dieser Node, ohne Cache-Pruning. */
    void sendDiff (bool force, const juce::String& onlyNodeUuid = {});

    //==========================================================================
    using Key = std::pair<juce::String, juce::String>;  // (nodeUuid, paramId)

    juce::ValueTree rootState;  // ref-counted Handle
    OscSendSettings& settings;
    std::unique_ptr<IOscSink> sink;

    std::map<Key, float> lastSent;  // nur Message Thread
    bool connected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSendService)
};

} // namespace conduit
