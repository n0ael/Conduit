#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "Core/LinkClock.h"
#include "Interfaces/IClockSlave.h"
#include "Interfaces/ILinkAudioClient.h"
#include "NetworkIOModule.h"

namespace conduit
{

//==============================================================================
/**
    Link Audio Send (CLAUDE.md 7.2): announced einen Audio-Kanal in der
    Link-Session (Kanal-Name == moduleId) und sendet den Block-Input als
    interleaved 16-bit-Audio mit TPDF-Dither. Output ist reines Pass-Through
    des Inputs — das Modul ist mitten in eine Kette patchbar.

    Sink-Lifecycle [Message Thread]:
      - Der GraphManager injiziert Clock + moduleId via ILinkAudioClient VOR
        prepareForGraph; der Sink entsteht in prepareToPlay (Größe in SAMPLES:
        samplesPerBlock × Kanäle). enableAudio-Refcount der LinkClock: dieses
        Modul zählt +1 bei Sink-Erzeugung, −1 bei Phase 1 bzw. Destruktion.
      - Rename (moduleIdRenamed) → sink.setName(), live zu den Peers.
      - Delete Phase 1 (releaseSessionResources, 5.3): der Audio-Thread wird
        sofort per rtSink-Atomic vom Sink getrennt; die Destruktion folgt
        nach dem Epoch-Handshake (unten), damit der Kanal ohne Race und
        praktisch sofort (≤ 1 Audio-Block) aus der Session verschwindet.

    Epoch-Handshake (Sink-Teardown vs. processBlock):
      processBlock inkrementiert blocksProcessed (seq_cst) und lädt DANACH
      rtSink. Phase 1 stored nullptr (seq_cst) und liest dann die Epoche.
      Beobachtet der Message Thread später blocksProcessed > Epoche, hat ein
      neuer Block begonnen — der hat den nullptr garantiert gesehen (seq_cst-
      Totalordnung: sein Inkrement liegt nach unserem Epoch-Read, also nach
      dem Store), und der ältere Block ist beendet (Callbacks sind sequenziell).
      Gewartet wird per AsyncUpdater-Self-Re-Dispatch (Muster 5.2 Schritt 3);
      läuft kein Audio, destruiert eine 100-ms-Deadline trotzdem.

    Audio-Pfad [Audio Thread, 3.1]: allocation-/lock-frei; Dither via inline
    LCG in den vorallokierten Member-Buffer; commit über
    Sink::commitFromClockState mit dem ClockState des Blocks (ClockBus).
    Der Commit-Status wird als Atomic für die UI gespiegelt.
*/
class LinkAudioSendModule final : public NetworkIOModule,
                                  public ILinkAudioClient,
                                  public IClockSlave,
                                  private juce::AsyncUpdater
{
public:
    LinkAudioSendModule();
    ~LinkAudioSendModule() override;

    static constexpr const char* staticModuleId = "link_audio_send";

    //==========================================================================
    // Pflicht-Methoden (CLAUDE.md 4.4)
    [[nodiscard]] juce::String getModuleId() const override;
    [[nodiscard]] juce::String getModuleDisplayName() const override;
    [[nodiscard]] int getStateVersion() const override;

    //==========================================================================
    // ILinkAudioClient [Message Thread]
    void setLinkAudioContext (LinkClock* clock, const juce::String& initialModuleId) override;
    void moduleIdRenamed (const juce::String& newModuleId) override;
    void releaseSessionResources() override;

    // IClockSlave [Message Thread, vor Graph-Aufnahme]
    void setClockBus (const ClockBus* bus) noexcept override;

    //==========================================================================
    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==========================================================================
    /** UI-Status (beliebiger Thread, atomic):
        offline   — kein Sink (kein Link-Kontext oder Phase 1 gelaufen)
        announced — Kanal in der Session sichtbar, aber letzter Block ohne
                    Commit (kein Subscriber, Overrun oder Audio steht)
        streaming — letzter Block erfolgreich committed.

        Grenze (bewusst, 7.2): Erkennung über commit-Aktivität — ein Overrun
        ist von „kein Subscriber" über die Link-API nicht unterscheidbar und
        fällt auf announced zurück; ohne laufendes Audio bleibt der Status
        beim letzten gemeldeten Wert bzw. announced. */
    enum class SendStatus : int { offline = 0, announced = 1, streaming = 2 };

    [[nodiscard]] SendStatus getSendStatusForUi() const noexcept;

    //==========================================================================
    // Message Thread — Diagnose/Tests
    [[nodiscard]] juce::String getSinkName() const;            // leer ohne Sink
    [[nodiscard]] bool isSinkRetirePending() const noexcept;   // Phase 1 lief, Handshake offen
    void flushPendingSinkRetirement();                         // Handshake synchron abschließen

    //==========================================================================
    /** Float → Int16 mit TPDF-Dither (CLAUDE.md 3.1/7.2): LCG-basiert,
        deterministisch pro Seed, ±1 LSB Dreieck um 0. Schreibt
        numFrames × numChannels Samples interleaved nach dest.
        Static + pure für die Dither-Statistik-Tests (13.4). */
    static void convertToInt16Tpdf (const float* const* channelData,
                                    int numChannels, int numFrames,
                                    std::int16_t* dest,
                                    std::uint32_t& ditherState) noexcept;

private:
    void handleAsyncUpdate() override;

    /** enableAudio(false) genau einmal pro Sink-Lebenszyklus — Phase 1 und
        Destruktor (Preset-Load/Shutdown ohne Phase 1) teilen sich den Pfad. */
    void disableAudioOnce();

    //==========================================================================
    // Message Thread
    juce::WeakReference<LinkClock> linkClock;  // Weak: Shutdown-Reihenfolge-Netz
    juce::String currentModuleId;
    std::unique_ptr<LinkClock::Sink> sink;
    std::unique_ptr<LinkClock::Sink> retiredSink;  // wartet auf den Epoch-Handshake
    bool audioEnabled = false;
    std::uint64_t retireEpoch = 0;
    std::uint32_t retireDeadlineMs = 0;

    static constexpr std::uint32_t retireGraceMs = 100;

    // Audio Thread liest; Injektion vor Graph-Aufnahme (4.2)
    const ClockBus* clockBus = nullptr;

    // Handshake-Paar (seq_cst, siehe Klassen-Doku)
    std::atomic<LinkClock::Sink*> rtSink { nullptr };
    std::atomic<std::uint64_t> blocksProcessed { 0 };

    // Nur Audio Thread
    std::vector<std::int16_t> interleavedBuffer;  // vorallokiert in prepareToPlay
    std::uint32_t ditherState = 0x6c078965u;

    std::atomic<int> sendStatus { static_cast<int> (SendStatus::offline) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LinkAudioSendModule)
};

} // namespace conduit
