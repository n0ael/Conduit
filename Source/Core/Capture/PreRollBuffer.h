#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include <juce_core/juce_core.h>

namespace conduit
{

//==============================================================================
/**
    Mono-Pre-Roll-Ring eines Capture-Kanals — läuft IMMER, unabhängig vom Gate.

    Hält die letzten preRollSeconds an rohem Input, damit ein Gate-Open
    rückwirkend Material "vor dem ersten Ton" liefert: der CaptureChannel
    kopiert den Inhalt amortisiert in den CaptureRingBuffer, sobald der
    BufferPool ein Segment publiziert hat. Bis dahin überbrückt dieser Ring
    die Pool-Latenz (60 s Pre-Roll >> ~200 ms Pool-Timer).

    Positionen sind absolut in SampleClock-Einheiten: ein Sample mit
    Position p liegt immer bei Index p % capacity. write() bekommt die
    Blockstart-Position mitgereicht; springt sie (erster Block nach einem
    Puffersatz-Swap, Aussetzer), wird der Fill-State verworfen — der alte
    Inhalt wäre positionsmäßig nicht mehr zusammenhängend.

    Threading: prepare() [Message Thread, BEVOR der Puffersatz im Audio-Pfad
    publiziert ist] ist die einzige Allokation. Danach fasst ausschließlich
    der Audio Thread den Ring an — Schreiber (write) und Leser (Pre-Roll-
    Übernahme via makeReadSpans) sind derselbe Thread, deshalb braucht es
    hier keine Atomics; die Thread-Grenze liegt eine Ebene höher im
    CaptureService-Set-Swap bzw. in den CaptureRingBuffer-Positionen.
*/
class PreRollBuffer
{
public:
    PreRollBuffer() = default;

    /** Bis zu zwei zusammenhängende Speicherstücke (Ring-Wrap) für die
        allocation-free Übergabe an CaptureRingBuffer::writeSpansAt(). */
    struct ReadSpans
    {
        const float* data1 = nullptr; int num1 = 0;
        const float* data2 = nullptr; int num2 = 0;
    };

    //==========================================================================
    /** [Message Thread, Set noch nicht publiziert] Einzige Allokation. */
    void prepare (int capacitySamples)
    {
        storage.assign (static_cast<size_t> (juce::jmax (0, capacitySamples)), 0.0f);
        endPosition  = 0;
        validSamples = 0;
    }

    /** [Audio Thread] Fill-State verwerfen (Invalidate) — keine Allokation. */
    void clear() noexcept { validSamples = 0; }

    //==========================================================================
    /** [Audio Thread] blockStart = SampleClock-Stand des ersten Samples. */
    void write (const float* src, int numSamples, std::uint64_t blockStart) noexcept
    {
        const auto capacity = getCapacity();
        if (src == nullptr || numSamples <= 0 || capacity <= 0)
            return;

        if (blockStart != endPosition)
            validSamples = 0;  // Positionssprung: Historie nicht mehr zusammenhängend

        if (numSamples > capacity)  // defensiv: nur die letzten capacity Samples
        {
            src        += numSamples - capacity;
            blockStart += static_cast<std::uint64_t> (numSamples - capacity);
            numSamples  = capacity;
        }

        const auto index = static_cast<int> (blockStart % static_cast<std::uint64_t> (capacity));
        const auto firstPart = juce::jmin (numSamples, capacity - index);
        std::memcpy (storage.data() + index, src,
                     sizeof (float) * static_cast<size_t> (firstPart));
        if (firstPart < numSamples)
            std::memcpy (storage.data(), src + firstPart,
                         sizeof (float) * static_cast<size_t> (numSamples - firstPart));

        endPosition  = blockStart + static_cast<std::uint64_t> (numSamples);
        validSamples = juce::jmin (validSamples + numSamples, capacity);
    }

    //==========================================================================
    // [Audio Thread] Lesegrenzen & Spans für die Übernahme
    [[nodiscard]] int getCapacity() const noexcept     { return static_cast<int> (storage.size()); }
    [[nodiscard]] int getValidSamples() const noexcept { return validSamples; }

    /** Absolute Position NACH dem letzten geschriebenen Sample. */
    [[nodiscard]] std::uint64_t getEndPosition() const noexcept { return endPosition; }

    /** Älteste noch gültige Position (== endPosition wenn leer). */
    [[nodiscard]] std::uint64_t getOldestValidPosition() const noexcept
    {
        return endPosition - static_cast<std::uint64_t> (validSamples);
    }

    /** Caller garantiert [startPos, startPos + numSamples) innerhalb des
        gültigen Fensters — der CaptureChannel leitet beides aus
        getOldestValidPosition()/getEndPosition() ab. */
    [[nodiscard]] ReadSpans makeReadSpans (std::uint64_t startPos, int numSamples) const noexcept
    {
        ReadSpans spans;
        const auto capacity = getCapacity();

        jassert (numSamples >= 0 && numSamples <= validSamples);
        jassert (startPos >= getOldestValidPosition());
        jassert (startPos + static_cast<std::uint64_t> (juce::jmax (0, numSamples)) <= endPosition);

        if (numSamples <= 0 || capacity <= 0)
            return spans;

        const auto index = static_cast<int> (startPos % static_cast<std::uint64_t> (capacity));
        spans.data1 = storage.data() + index;
        spans.num1  = juce::jmin (numSamples, capacity - index);

        if (spans.num1 < numSamples)
        {
            spans.data2 = storage.data();
            spans.num2  = numSamples - spans.num1;
        }

        return spans;
    }

private:
    std::vector<float> storage;
    std::uint64_t endPosition = 0;  // absolute Position nach dem letzten Sample
    int validSamples = 0;           // zusammenhängend gültig bis endPosition

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreRollBuffer)
};

} // namespace conduit
