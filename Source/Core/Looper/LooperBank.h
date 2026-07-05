#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "BarSampleAnchors.h"
#include "Core/Capture/CaptureService.h"
#include "Interfaces/IClockSource.h"
#include "LooperClip.h"
#include "LooperClipMath.h"
#include "LooperMath.h"
#include "Util/SpscQueue.h"

namespace conduit
{

//==============================================================================
/**
    Multi-Looper-Playback des Looper-Vollausbaus (M2) — Nachfolger der
    LooperEngine (Retro-Looper B5, git-Historie), deren Playhead/Wrap-
    Crossfade/Commit-Pfad hier 1:1 weiterleben. Herleitungen (Wall-Clock-
    Jitter-Lektion, Snap-Declick, Duck, Lead-in) stehen in CLAUDE.md 10.0
    „Retro-Looper" und den Kommentaren unten. Engine-Level wie das Metronom,
    bewusst OHNE EngineProcessor-Abhängigkeit (nur ClockState +
    CaptureService + BarSampleAnchors + AudioBuffer per Parameter) — das
    spätere patchbare LooperModule hostet dieselbe Klasse.

    Struktur: bis zu maxLoopers × maxTracks TrackPlayer, jeder mit
    voicesPerTrack Crossfade-Voices. Voices REFERENZIEREN LooperClips
    (right-sized beim Commit, ClipStore auf dem MT) statt eigene
    60-s-Buffer zu tragen — die ~46-MB-Prealloc der LooperEngine entfällt.

    Threading (strikt, CLAUDE.md 3.1):
      - Voices/Playhead sind AUDIO-only-Zustand; der MT fasst sie nie an.
      - MT → Audio: SpscQueue<ClipCommand> (activate/deleteClip/stopTrack)
        + Atomics (stopAll, anchor). Clips sind beim Push fertig gebaut.
      - Audio → MT: SpscQueue<LooperClip*> (Retire-Quittungen). Ein
        deleteClip wandert IMMER durch den Audio-Thread: erst wenn der die
        Quittung pusht, ist garantiert keine Voice-Referenz mehr da —
        serviceMessageThread() gibt dann frei (free nie im Audio-Thread).
        exportPins (Save-Geste M9) verzögern die Freigabe zusätzlich.
      - Drain-Guard: der Audio-Thread konsumiert Commands nur, solange die
        Retire-Queue Luft für alle denkbaren Quittungen des Blocks hat —
        Überschuss wartet einen Block, nichts geht verloren.

    RAM-Konto: right-sized Clips statt Prealloc; die Summe aller lebenden
    Clips (Store + Graveyard) ist auf ramBudgetBytes begrenzt — ein Commit
    über Budget schlägt sauber fehl (kein OOM im Live-Set). Default 1,5 GB
    (~250 Clips à 2 Takte @ 120 BPM/48 kHz).

    M2-Scope: commitAndPlay ersetzt den Clip eines Tracks (Paritäts-
    verhalten zur LooperEngine, ein Loop pro Track); Slot-/Target-Logik
    kommt mit dem LooperSessionModel (M4), Quantisierung/Rate/Reverse als
    Verhalten mit M3 (die Clip-Parameter-Mechanik steht bereits).
*/
class LooperBank
{
public:
    static constexpr int maxLoopers = 4;
    static constexpr int maxTracks  = 4;
    static constexpr int voicesPerTrack = 3;   // Crossfade + Doppel-Re-Commit

    static constexpr double maxLoopSeconds   = 60.0;
    static constexpr double crossfadeSeconds = 0.005;

    // Playhead-Konstanten — Herleitung siehe LooperEngine-Doku (übernommen)
    static constexpr double maxSlewRatio       = 0.002;
    static constexpr double snapThresholdBeats = 0.15;
    static constexpr int    snapConfirmBlocks  = 2;

    static constexpr std::int64_t defaultRamBudgetBytes = 1'500'000'000;

    LooperBank();

    /** [Message Thread, Audio steht — prepareToPlay] verwirft alle Clips
        und Voices (SampleRate-Wechsel invalidiert Positionen/Inhalte). */
    void prepare (double sampleRate);

    /** [Message Thread] Die letzten `bars` kompletten Takte in den Track
        committen und sofort phasenstarr abspielen — ersetzt dessen
        laufenden Clip glitch-frei (Voice-Crossfade). Fehlerfälle: keine
        Quelle, zu wenig Historie, Anker-Fenster, > maxLoopSeconds,
        RAM-Budget, Command-Queue voll. */
    [[nodiscard]] juce::Result commitAndPlay (int looperIndex, int trackIndex,
                                              int bars, const CaptureService& capture,
                                              int leftIndex, int rightIndex,
                                              const BarSampleAnchors& anchors);

    /** [Message Thread] Alle Tracks mit 5-ms-Fade stoppen (Clips bleiben). */
    void stopAll() noexcept;

    /** [Message Thread] Retire-Quittungen einsammeln und Graveyard-Clips
        freigeben (exportPins beachtet) — vom Editor-Timer und vor jedem
        Commit gerufen. */
    void serviceMessageThread();

    /** Stereo-Anker: Loops auf Kanäle pairIndex*2 / +1 (global, M2). */
    void setAnchor (int pairIndex) noexcept
    {
        anchor.store (juce::jmax (0, pairIndex), std::memory_order_relaxed);
    }

    /** [Audio Thread] Alle Loops auf die Anker-Kanäle addieren — Kontrakt
        identisch zur LooperEngine (blockStartSample = SampleClock-Position
        des Block-Anfangs, anchors = die Commit-Instanz). */
    void process (juce::AudioBuffer<float>& buffer, int numOutputChannels,
                  const ClockState& clock, std::uint64_t blockStartSample,
                  const BarSampleAnchors& anchors) noexcept;

    //==========================================================================
    // Status [beliebiger Thread]

    [[nodiscard]] bool isPlaying() const noexcept
    {
        return playingFlag.load (std::memory_order_relaxed);
    }

    /** Takte des zuletzt committeten Loops (0 = keiner) — Paritäts-API. */
    [[nodiscard]] int getLoopBars() const noexcept
    {
        return isPlaying() ? committedBars.load (std::memory_order_relaxed) : 0;
    }

    /** Diagnose: Playhead-Re-Syncs (Duck-Snaps) seit prepare. */
    [[nodiscard]] std::uint32_t getSnapCount() const noexcept
    {
        return snapCount.load (std::memory_order_relaxed);
    }

    /** RAM-Konto (Dev-Diagnose). */
    [[nodiscard]] std::int64_t getRamBytesUsed() const noexcept
    {
        return ramBytesUsed.load (std::memory_order_relaxed);
    }

    /** [Message Thread] Budget-Grenze (Tests / spätere Settings). */
    void setRamBudgetBytes (std::int64_t bytes) noexcept { ramBudgetBytes = bytes; }

private:
    //==========================================================================
    struct ClipCommand
    {
        enum class Type : int { activate = 0, deleteClip, stopTrack };

        Type type = Type::activate;
        int looper = 0;
        int track = 0;
        LooperClip* clip = nullptr;
    };

    // Audio-only: Voice referenziert einen Clip; retireOnEnd = dieser Voice
    // gehört die Retire-Quittung des Clips (deleteClip-Protokoll)
    struct Voice
    {
        LooperClip* clip = nullptr;
        float gain = 0.0f;
        bool fading = false;
        bool retireOnEnd = false;
    };

    struct TrackPlayer
    {
        std::array<Voice, static_cast<std::size_t> (voicesPerTrack)> voices;
    };

    // MT-only: Graveyard-Eintrag wartet auf die Audio-Quittung + Pins
    struct Grave
    {
        std::unique_ptr<LooperClip> clip;
        bool audioReleased = false;
    };

    //==========================================================================
    // [Audio] Kommandos des Blocks übernehmen (Drain-Guard, Klassendoku)
    void drainCommands() noexcept;
    void handleActivate (const ClipCommand& command) noexcept;
    void handleDelete (const ClipCommand& command) noexcept;

    /** [Audio] true, wenn irgendeine Voice den Clip (noch) referenziert. */
    [[nodiscard]] bool anyVoiceReferences (const LooperClip* clip) const noexcept;

    /** [Audio] Retire-Pflicht eines Clips übergeben (an eine noch
        referenzierende Voice) oder quittieren (Retire-Queue). false nur
        bei voller Queue — der Caller parkt bis zum nächsten Block. */
    bool transferOrRetire (LooperClip* clip) noexcept;

    /** Clip aus dem Store in den Graveyard verschieben [MT]. */
    void moveToGraveyard (LooperClip* clip);

    /** Kanal chunked in den Clip-Buffer lesen (Export-Halte-Protokoll,
        1:1 LooperEngine) [MT]. */
    static void readChannelChunked (const CaptureChannel* channel,
                                    std::uint64_t startPosition,
                                    float* dest, int numSamples);

    /** Sample eines Clips an (fraktionaler) Content-Position — linear
        interpoliert, mit Wrap-Crossfade auf den Lead-in (1:1
        LooperEngine::renderVoiceSample, nur clip-lokale Maße). [Audio] */
    [[nodiscard]] static float renderClipSample (const LooperClip& clip, int channel,
                                                 double contentPosition) noexcept;

    //==========================================================================
    // Audio-only: Player-Matrix + sample-kontinuierlicher Beat-Playhead
    std::array<std::array<TrackPlayer, static_cast<std::size_t> (maxTracks)>,
               static_cast<std::size_t> (maxLoopers)> players;

    double playheadBeat  = 0.0;
    bool   playheadValid = false;
    int    snapPendingCount = 0;
    bool   snapDucking = false;
    float  duckGain = 1.0f;

    // MT → Audio / Audio → MT
    SpscQueue<ClipCommand> commands { 1024 };
    SpscQueue<LooperClip*> retired  { 1024 };

    std::atomic<int>  anchor { 0 };
    std::atomic<bool> stopAllRequested { false };
    std::atomic<bool> playingFlag { false };
    std::atomic<int>  committedBars { 0 };
    std::atomic<std::uint32_t> snapCount { 0 };
    std::atomic<std::int64_t>  ramBytesUsed { 0 };

    // MT-only: Clip-Besitz + Buchführung
    std::vector<std::unique_ptr<LooperClip>> store;
    std::vector<Grave> graveyard;
    std::array<std::array<LooperClip*, static_cast<std::size_t> (maxTracks)>,
               static_cast<std::size_t> (maxLoopers)> mtActiveClip {};
    std::uint32_t nextClipId = 0;
    std::int64_t ramBudgetBytes = defaultRamBudgetBytes;

    // MT-only (prepare)
    double preparedSampleRate = 0.0;
    int crossfadeSamples = 0;
    int maxLoopSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperBank)
};

} // namespace conduit
