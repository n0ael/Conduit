#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>

#include <juce_core/juce_core.h>

#include "PreRollBuffer.h"

namespace conduit
{

//==============================================================================
/**
    Mono-Aufnahme-Ring eines Capture-Kanals — Speicher kommt aus dem BufferPool.

    Der Ring allokiert selbst NIE: attach() bekommt ein Pool-Segment
    [Audio Thread], detach() gibt es zurück. Jede Position ist absolut in
    SampleClock-Einheiten — Sample p liegt bei Index
    (p − startSamplePosition) % capacity. Damit ist jede Aufnahmeposition
    exakt rekonstruierbar, und Pre-Roll-Übernahme (rückwärts) und Live-Strom
    (vorwärts) können denselben Ring parallel füllen, ohne sich um
    Speicher-Reihenfolge zu kümmern.

    SPSC-Disziplin analog conduit::SpscQueue: der Audio Thread schreibt
    (writeAt/writeSpansAt) und publiziert mit publishEnd() (release) den
    Schreib-Cursor; ein Leser auf einem anderen Thread liest zuerst
    getEndPosition() (acquire) und konsumiert dann ausschließlich dahinter
    liegende Bereiche via copyOut(). attach()/detach() dürfen nur laufen,
    solange kein fremder Thread liest — Tests sind single-threaded, der
    Export (Capture-Baustein 5) definiert sein eigenes Halte-Protokoll.

    Gültigkeit (Übernahme-Lücke, Wrap-Verdrängung, Aussetzer) bewertet der
    CaptureChannel über getReadableRange() — der Ring selbst ist reiner
    positionsadressierter Speicher.
*/
class CaptureRingBuffer
{
public:
    CaptureRingBuffer() = default;

    //==========================================================================
    // [Audio Thread]

    void attach (float* storageToUse, int capacitySamples, std::uint64_t startPosition) noexcept
    {
        storage  = storageToUse;
        capacity = juce::jmax (0, capacitySamples);
        startSamplePosition.store (startPosition, std::memory_order_relaxed);
        endPosition.store (startPosition, std::memory_order_release);
    }

    /** Gibt den Storage-Pointer an den Caller zurück (→ BufferPool). */
    float* detach() noexcept
    {
        auto* released = storage;
        storage  = nullptr;
        capacity = 0;
        startSamplePosition.store (0, std::memory_order_relaxed);
        endPosition.store (0, std::memory_order_release);
        return released;
    }

    [[nodiscard]] bool hasStorage() const noexcept { return storage != nullptr; }
    [[nodiscard]] int getCapacity() const noexcept { return capacity; }

    /** Schreibt an absoluter Position (>= startSamplePosition), mit Wrap. */
    void writeAt (std::uint64_t position, const float* src, int numSamples) noexcept
    {
        if (storage == nullptr || src == nullptr || numSamples <= 0)
            return;

        jassert (numSamples <= capacity);
        jassert (position >= startSamplePosition.load (std::memory_order_relaxed));

        const auto offset = position - startSamplePosition.load (std::memory_order_relaxed);
        const auto index = static_cast<int> (offset % static_cast<std::uint64_t> (capacity));
        const auto firstPart = juce::jmin (numSamples, capacity - index);
        std::memcpy (storage + index, src, sizeof (float) * static_cast<size_t> (firstPart));
        if (firstPart < numSamples)
            std::memcpy (storage, src + firstPart,
                         sizeof (float) * static_cast<size_t> (numSamples - firstPart));
    }

    /** Allocation-free Übernahme direkt aus den PreRollBuffer-Spans. */
    void writeSpansAt (std::uint64_t position, const PreRollBuffer::ReadSpans& spans) noexcept
    {
        writeAt (position, spans.data1, spans.num1);
        writeAt (position + static_cast<std::uint64_t> (juce::jmax (0, spans.num1)),
                 spans.data2, spans.num2);
    }

    /** Publiziert den Live-Schreibstand (release) — danach dürfen Leser
        alle zuvor geschriebenen Samples bis newEndPosition konsumieren. */
    void publishEnd (std::uint64_t newEndPosition) noexcept
    {
        endPosition.store (newEndPosition, std::memory_order_release);
    }

    //==========================================================================
    // [beliebiger Thread]

    /** Absoluter Anker des ersten Samples (SampleClock-Stand). */
    [[nodiscard]] std::uint64_t getStartSamplePosition() const noexcept
    {
        return startSamplePosition.load (std::memory_order_relaxed);
    }

    /** Absolute Position nach dem letzten publizierten Live-Sample. */
    [[nodiscard]] std::uint64_t getEndPosition() const noexcept
    {
        return endPosition.load (std::memory_order_acquire);
    }

    //==========================================================================
    /** [Leser-Thread hinter dem Schreib-Cursor — Kontrakt siehe Klassendoku]
        Bereichsprüfung macht der CaptureChannel (getReadableRange). */
    void copyOut (std::uint64_t position, float* dest, int numSamples) const noexcept
    {
        if (storage == nullptr || dest == nullptr || numSamples <= 0)
            return;

        jassert (numSamples <= capacity);
        jassert (position >= startSamplePosition.load (std::memory_order_relaxed));

        const auto offset = position - startSamplePosition.load (std::memory_order_relaxed);
        const auto index = static_cast<int> (offset % static_cast<std::uint64_t> (capacity));
        const auto firstPart = juce::jmin (numSamples, capacity - index);
        std::memcpy (dest, storage + index, sizeof (float) * static_cast<size_t> (firstPart));
        if (firstPart < numSamples)
            std::memcpy (dest + firstPart, storage,
                         sizeof (float) * static_cast<size_t> (numSamples - firstPart));
    }

private:
    float* storage = nullptr;  // nicht besessen — BufferPool-Segment
    int capacity = 0;

    std::atomic<std::uint64_t> startSamplePosition { 0 };
    std::atomic<std::uint64_t> endPosition { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureRingBuffer)
};

} // namespace conduit
