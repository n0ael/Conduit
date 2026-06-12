#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include <juce_core/juce_core.h>

#include "InputMeter.h"  // MAX_CAPTURE_CHANNELS
#include "Util/SpscQueue.h"

namespace conduit
{

//==============================================================================
/**
    Segment-Pool für CaptureRingBuffer-Speicher — RAM erst bei Bedarf.

    Der Message Thread besitzt alle Segmente und hält preallocTarget Stück
    frei vor; der volle Ausbau (numChannels × Ring) wird nie auf Vorrat
    allokiert. Segmente sind bewusst uninitialisiert (HeapBlock/malloc):
    kein Gigabyte-Memset in prepareToPlay, und Leser sind über die
    ReadableRange des CaptureChannel geschützt.

    Handshake (strikt SPSC, wie conduit::SpscQueue):
      - requestSegment()   [Audio]   — atomarer Anforderungszähler
      - service()          [Message] — bedient Anfragen, publiziert Pointer
                                       über die Publish-Queue, sammelt
                                       Rückgaben ein, hält das Vorhalteziel
      - tryClaimSegment()  [Audio]   — holt einen publizierten Pointer ab
      - releaseSegment()   [Audio]   — gibt ein Segment über die
                                       Release-Queue zurück

    Bis ein Segment publiziert ist (≤ ein Guard-Timer-Intervall), überbrückt
    der PreRollBuffer des Kanals die Wartezeit — bei 60 s Pre-Roll gegen
    ~200 ms Pool-Latenz geht dabei nichts verloren, die Übernahme holt das
    Material rückwirkend aus dem Pre-Roll.

    Ein angefordertes, aber nie geclaimtes Segment (Gate schloss vor der
    Publikation) bleibt in der Publish-Queue und bedient das nächste
    Gate-Open sofort — Segmente sind untereinander austauschbar.

    prepare() nur, solange der Puffersatz nicht im Audio-Pfad publiziert ist
    (CaptureService baut pro Reallokation einen komplett neuen Satz).
*/
class BufferPool
{
public:
    BufferPool() = default;

    //==========================================================================
    // [Message Thread]

    void prepare (int segmentSamplesToUse, int maxSegmentsToUse, int preallocSegments);

    /** Periodisch vom RAM-Wächter-Timer gerufen (CaptureService). */
    void service();

    [[nodiscard]] std::int64_t getAllocatedBytes() const noexcept
    {
        return allocatedBytes.load (std::memory_order_relaxed);
    }

    /** true, wenn service() Anfragen mangels RAM-Budget nicht bedienen
        konnte — der RAM-Wächter gibt dann gehaltene Kanäle frei. */
    [[nodiscard]] bool isExhausted() const noexcept
    {
        return exhausted.load (std::memory_order_relaxed);
    }

    [[nodiscard]] int getNumSegments() const noexcept    { return static_cast<int> (segments.size()); }
    [[nodiscard]] int getSegmentSamples() const noexcept { return segmentSamples; }
    [[nodiscard]] int getMaxSegments() const noexcept    { return maxSegments; }

    //==========================================================================
    // [Audio Thread]

    /** Wait-free: Anforderung vormerken, service() publiziert später. */
    void requestSegment() noexcept
    {
        pendingRequests.fetch_add (1, std::memory_order_relaxed);
    }

    /** Wait-free: publiziertes Segment abholen, nullptr wenn keins da. */
    [[nodiscard]] float* tryClaimSegment() noexcept
    {
        float* segment = nullptr;
        publishQueue.pop (segment);
        return segment;
    }

    /** Wait-free: Segment zurückgeben — service() sammelt es ein. */
    void releaseSegment (float* segment) noexcept
    {
        if (segment != nullptr)
            releaseQueue.push (segment);
    }

private:
    float* allocateSegment();           // nullptr wenn maxSegments erreicht
    void freeSegment (float* segment);

    int segmentSamples = 0;
    int maxSegments = 0;
    int preallocTarget = 0;

    std::vector<juce::HeapBlock<float>> segments;  // MT-Besitz aller Segmente
    std::vector<float*> freeSegments;              // MT: unbenutzte Segmente

    std::atomic<int> pendingRequests { 0 };
    std::atomic<std::int64_t> allocatedBytes { 0 };
    std::atomic<bool> exhausted { false };

    SpscQueue<float*> publishQueue { MAX_CAPTURE_CHANNELS };  // MT → Audio
    SpscQueue<float*> releaseQueue { MAX_CAPTURE_CHANNELS };  // Audio → MT

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BufferPool)
};

} // namespace conduit
