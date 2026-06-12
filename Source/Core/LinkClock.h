#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

#include <juce_events/juce_events.h>

#include "Interfaces/IClockSource.h"

namespace conduit
{

//==============================================================================
/**
    Clock-Master auf Basis einer Ableton-Link-Session (CLAUDE.md 2/13.1).

    Pimpl: der Link-/asio-Header bleibt in der .cpp — er zieht plattform-
    spezifische Netzwerk-Header, die nicht in Projekt-Header gehören.

    Basis ist ableton::LinkAudio, das ableton::Link vollständig ERSETZT
    (CLAUDE.md 7.2 — nie beide Klassen parallel). Timing-Verhalten ist
    identisch zu Link; Audio-Sharing ist initial deaktiviert und wird erst
    vom ersten Send-Modul via enableAudio(true) aktiviert.

    Link ist ab Konstruktion aktiv (Peer-Discovery im lokalen Netz) inkl.
    Start/Stop-Sync. Ohne Peers verhält sich die Session wie eine lokale
    Clock — der Session-Beat läuft auf der Wall-Clock weiter.

    ChangeBroadcaster: benachrichtigt (auf dem Message Thread), wenn sich
    die in der Session verfügbaren Audio-Kanäle ändern — der zugrunde
    liegende ChannelsChangedCallback kommt auf einem Link-Thread und wird
    via MessageManager::callAsync gemarshallt (CLAUDE.md 7.2 Threading).

    Thread-Ownership:
      - ctor/dtor, setTempo(), getTempo(), getNumPeers() → Message Thread
        (Link kapselt die Synchronisation mit seinen Netzwerk-Threads)
      - enableAudio(), peerName(), setPeerName(), createSink()
                              → Message Thread
      - isAudioEnabled()      → beliebiger Thread (Link-Garantie, RT-safe)
      - prepare()             → Message Thread, Audio gestoppt
      - captureClockState()   → Audio Thread, lock-free (Link-Garantie)
*/
class LinkClock final : public IClockSource,
                        public juce::ChangeBroadcaster
{
public:
    explicit LinkClock (double initialBpm = 120.0,
                        const juce::String& peerName = "Conduit");
    ~LinkClock() override;

    /** Vor Audio-Start (EngineProcessor::prepareToPlay). */
    void prepare (double sampleRate) noexcept;

    //==========================================================================
    // Message Thread
    void setTempo (double bpm);
    [[nodiscard]] double getTempo() const;
    [[nodiscard]] std::size_t getNumPeers() const;

    //==========================================================================
    // Link Audio (CLAUDE.md 7.2) — Message Thread

    /** Refcount-Semantik: jedes aktive Send-Modul ruft enableAudio(true) bei
        Aktivierung und enableAudio(false) bei Deaktivierung (Phase 1 des
        zweiphasigen Delete). Link-Audio ist enabled, solange der Zähler > 0
        ist — das erste Modul aktiviert, das letzte deaktiviert.
    */
    void enableAudio (bool shouldBeEnabled);

    /** Beliebiger Thread, RT-safe (Link-Garantie). */
    [[nodiscard]] bool isAudioEnabled() const;

    /** Peer-Name zur Identifikation in der Link-Session (Default "Conduit"). */
    [[nodiscard]] juce::String peerName() const;
    void setPeerName (const juce::String& name);

    //==========================================================================
    /**
        Opaker Handle auf einen announceten Audio-Kanal (Sink) der Session.

        Design-Entscheidung: createSink() liefert diesen Pimpl-Wrapper statt
        ableton::LinkAudioSink direkt — so bleibt der Link-/asio-Header in
        der .cpp gekapselt (IWYU-Falle 7.2) und kein Projekt-Header zieht
        Netzwerk-Header. Die RT-Schreib-API (BufferHandle::commit) kommt im
        nächsten Schritt mit dem LinkAudioSendModule auf diesen Wrapper.

        Lifecycle: ein Sink darf die LinkClock nicht überleben. Kanal-Name
        == moduleId (7.2); der announcete Kanal verschwindet mit der
        Destruktion des Sinks (Phase 1 des zweiphasigen Delete — sonst
        Zombie-Kanäle bei den Peers).

        Alle Methoden: Message Thread.
    */
    class Sink final
    {
    public:
        ~Sink();

        [[nodiscard]] juce::String getName() const;

        /** Rename wird live zu den Peers propagiert (7.2). */
        void setName (const juce::String& newName);

    private:
        friend class LinkClock;
        struct Impl;
        explicit Sink (std::unique_ptr<Impl> impl);
        std::unique_ptr<Impl> impl;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sink)
    };

    /** Announced einen Audio-Kanal in der Session. maxNumSamples ist in
        SAMPLES, nicht Frames: samplesPerBlock * numChannels (7.2).
    */
    [[nodiscard]] std::unique_ptr<Sink> createSink (const juce::String& name,
                                                    std::size_t maxNumSamples);

    //==========================================================================
    // Audio Thread
    [[nodiscard]] ClockState captureClockState (int numSamples) noexcept override;

private:
    static constexpr double quantum = 4.0;  // 4/4 — Phase-Sync auf Takt-Ebene

    struct Impl;
    std::unique_ptr<Impl> impl;

    std::atomic<double> currentSampleRate { 48000.0 };
    int audioRefCount = 0;  // nur Message Thread (enableAudio)

    JUCE_DECLARE_WEAK_REFERENCEABLE (LinkClock)
};

} // namespace conduit
