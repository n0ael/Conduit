#include "BufferPool.h"

#include <algorithm>

namespace conduit
{

void BufferPool::prepare (int segmentSamplesToUse, int maxSegmentsToUse, int preallocSegments)
{
    // Kein Handshake nötig: der Puffersatz ist noch nicht im Audio-Pfad
    // publiziert (CaptureService-Kontrakt), die Queues sind leer/exklusiv
    float* stale = nullptr;
    while (publishQueue.pop (stale)) {}
    while (releaseQueue.pop (stale)) {}

    segments.clear();
    freeSegments.clear();
    pendingRequests.store (0, std::memory_order_relaxed);
    allocatedBytes.store (0, std::memory_order_relaxed);
    exhausted.store (false, std::memory_order_relaxed);

    segmentSamples = juce::jmax (0, segmentSamplesToUse);
    maxSegments    = juce::jlimit (0, MAX_CAPTURE_CHANNELS, maxSegmentsToUse);
    preallocTarget = juce::jlimit (0, maxSegments, preallocSegments);

    for (int i = 0; i < preallocTarget; ++i)
        if (auto* segment = allocateSegment())
            freeSegments.push_back (segment);
}

//==============================================================================
void BufferPool::service()
{
    // Rückläufer einsammeln (Release-Queue: Audio produziert, MT konsumiert)
    float* returned = nullptr;
    while (releaseQueue.pop (returned))
        freeSegments.push_back (returned);

    // Anfragen bedienen — Reihenfolge egal, Segmente sind austauschbar
    while (pendingRequests.load (std::memory_order_relaxed) > 0)
    {
        float* segment = nullptr;
        if (! freeSegments.empty())
        {
            segment = freeSegments.back();
            freeSegments.pop_back();
        }
        else
        {
            segment = allocateSegment();
        }

        if (segment == nullptr)
            break;  // RAM-Budget erschöpft — der RAM-Wächter gibt Kanäle frei

        if (! publishQueue.push (segment))
        {
            freeSegments.push_back (segment);  // Queue voll (theoretisch)
            break;
        }

        pendingRequests.fetch_sub (1, std::memory_order_relaxed);
    }

    exhausted.store (pendingRequests.load (std::memory_order_relaxed) > 0,
                     std::memory_order_relaxed);

    // Vorhalteziel: Überschuss abbauen ("RAM erst bei Bedarf"), Fehlbestand
    // nachallokieren, damit das nächste Gate-Open sofort bedient werden kann
    while (static_cast<int> (freeSegments.size()) > preallocTarget)
    {
        freeSegment (freeSegments.back());
        freeSegments.pop_back();
    }

    while (static_cast<int> (freeSegments.size()) < preallocTarget)
    {
        auto* segment = allocateSegment();
        if (segment == nullptr)
            break;
        freeSegments.push_back (segment);
    }
}

//==============================================================================
float* BufferPool::allocateSegment()
{
    if (segmentSamples <= 0 || static_cast<int> (segments.size()) >= maxSegments)
        return nullptr;

    // Bewusst uninitialisiert (siehe Klassendoku): kein Memset über
    // hunderte Megabyte, Leser sind über die ReadableRange geschützt
    segments.emplace_back();
    segments.back().malloc (static_cast<size_t> (segmentSamples));
    allocatedBytes.fetch_add (static_cast<std::int64_t> (segmentSamples)
                                  * static_cast<std::int64_t> (sizeof (float)),
                              std::memory_order_relaxed);
    return segments.back().get();
}

void BufferPool::freeSegment (float* segment)
{
    const auto match = std::find_if (segments.begin(), segments.end(),
                                     [segment] (const juce::HeapBlock<float>& owned)
                                     { return owned.get() == segment; });
    if (match != segments.end())
    {
        segments.erase (match);
        allocatedBytes.fetch_sub (static_cast<std::int64_t> (segmentSamples)
                                      * static_cast<std::int64_t> (sizeof (float)),
                                  std::memory_order_relaxed);
    }
}

} // namespace conduit
