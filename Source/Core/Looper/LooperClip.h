#pragma once

#include <atomic>
#include <cstdint>

#include <juce_audio_basics/juce_audio_basics.h>

namespace conduit
{

//==============================================================================
/**
    Ein committeter Loop-Clip des Looper-Vollausbaus (M2) — right-sized beim
    Commit allokiert (Message Thread), per Pointer an den Audio-Thread
    gereicht (LooperBank-Command-Queue), nie auf dem Audio-Thread freigegeben
    (Retire-Queue → serviceMessageThread).

    Buffer-Layout (Muster LooperEngine-Voice): [0, crossfadeSamples) =
    Lead-in (Material VOR dem Loop-Start, fürs Wrap-Blending),
    [crossfadeSamples, crossfadeSamples + numContentSamples) = der Content.

    Feld-Klassen:
      - KONSTANT ab Commit (MT schreibt vor der Publikation, danach read-only
        auf beiden Threads): buffer, numContentSamples, crossfadeSamples,
        contentBeats, samplesPerBeatRecorded, commitStartSample, clipId,
        commitBars.
      - MUTABEL (MT schreibt, Audio liest blockkonstant am Blockanfang):
        anchorBeat/lengthBeats/rate/reversed/windowOffsetBeats — die
        Clip-Phase-Parameter (LooperClipMath). paramVersion inkrementiert
        der MT NACH einem zusammengehörigen Update; der Audio-Thread
        re-ankert phasen-kontinuierlich, wenn sich die Version ändert (M3).
      - exportPins: Save-Geste (M9) hält den Clip am Leben — die Freigabe
        eines Graveyard-Clips wartet, bis der Zähler 0 ist.
*/
struct LooperClip
{
    LooperClip() = default;

    juce::AudioBuffer<float> buffer;

    // Konstant ab Commit
    int    numContentSamples = 0;
    int    crossfadeSamples  = 0;
    double contentBeats = 0.0;
    double samplesPerBeatRecorded = 0.0;
    std::uint64_t commitStartSample = 0;
    std::uint32_t clipId = 0;
    int    commitBars = 0;

    // Mutabel [MT schreibt, Audio liest blockkonstant]
    std::atomic<double> anchorBeat { 0.0 };
    std::atomic<double> lengthBeats { 0.0 };
    std::atomic<double> rate { 1.0 };
    std::atomic<double> windowOffsetBeats { 0.0 };
    std::atomic<bool>   reversed { false };
    std::atomic<std::uint32_t> paramVersion { 0 };

    // Save-Geste (M9): > 0 = Export liest gerade aus dem Buffer
    std::atomic<int> exportPins { 0 };

    /** RAM-Konto-Beitrag (LooperBank-Budget). */
    [[nodiscard]] std::int64_t allocatedBytes() const noexcept
    {
        return static_cast<std::int64_t> (buffer.getNumChannels())
                 * static_cast<std::int64_t> (buffer.getNumSamples())
                 * static_cast<std::int64_t> (sizeof (float))
             + static_cast<std::int64_t> (sizeof (LooperClip));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperClip)
};

} // namespace conduit
