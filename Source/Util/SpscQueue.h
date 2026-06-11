#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <type_traits>
#include <vector>

namespace conduit
{

//==============================================================================
/**
    Lock-free Single-Producer/Single-Consumer-Ringbuffer (CLAUDE.md 3.1, 6.1).

    Übergibt trivially-copyable Messages (z.B. Parameter-Updates) über eine
    Thread-Grenze — wait-free, ohne Mutex, ohne Allokation in push()/pop().
    Die einzige Allokation passiert im Konstruktor (Message Thread, vor dem
    Einsatz im Audio-Pfad).

    Thread-Ownership (strikt SPSC — je Seite genau ein Thread):
      - push(), getNumFree()   → NUR Producer-Thread (Netzwerk/Message)
      - pop(), getNumReady()   → NUR Consumer-Thread (Audio)
      - Konstruktor/Destruktor → Message Thread, Queue außer Betrieb

    Voll-/Leer-Verhalten: push() auf voller Queue gibt false zurück und
    überschreibt nichts — die Drop-Strategie entscheidet der Caller.
*/
template <typename T>
class SpscQueue final
{
    static_assert (std::is_trivially_copyable_v<T>,
                   "SpscQueue übergibt Slots per memcpy-Semantik — T muss trivially copyable sein");

public:
    /** Kapazität wird auf die nächste Zweierpotenz aufgerundet (Index-Maske). */
    explicit SpscQueue (int minimumCapacity)
        : capacity (static_cast<juce::uint64> (juce::nextPowerOfTwo (juce::jmax (1, minimumCapacity)))),
          slots (static_cast<size_t> (capacity))
    {
    }

    [[nodiscard]] int getCapacity() const noexcept { return static_cast<int> (capacity); }

    //==========================================================================
    // Producer-Thread

    /** Wait-free. false wenn voll — Element wird dann verworfen. */
    bool push (const T& value) noexcept
    {
        const auto currentHead = head.load (std::memory_order_relaxed);

        if (currentHead - tail.load (std::memory_order_acquire) == capacity)
            return false;  // voll

        slots[static_cast<size_t> (currentHead & (capacity - 1))] = value;
        head.store (currentHead + 1, std::memory_order_release);
        return true;
    }

    /** Freie Slots aus Producer-Sicht (Momentaufnahme). */
    [[nodiscard]] int getNumFree() const noexcept
    {
        return static_cast<int> (capacity - (head.load (std::memory_order_relaxed)
                                             - tail.load (std::memory_order_acquire)));
    }

    //==========================================================================
    // Consumer-Thread

    /** Wait-free. false wenn leer — result bleibt dann unverändert. */
    bool pop (T& result) noexcept
    {
        const auto currentTail = tail.load (std::memory_order_relaxed);

        if (head.load (std::memory_order_acquire) == currentTail)
            return false;  // leer

        result = slots[static_cast<size_t> (currentTail & (capacity - 1))];
        tail.store (currentTail + 1, std::memory_order_release);
        return true;
    }

    /** Wartende Elemente aus Consumer-Sicht (Momentaufnahme). */
    [[nodiscard]] int getNumReady() const noexcept
    {
        return static_cast<int> (head.load (std::memory_order_acquire)
                                 - tail.load (std::memory_order_relaxed));
    }

private:
    const juce::uint64 capacity;
    std::vector<T> slots;

    // Monoton wachsende Indizes, Slot = Index & (capacity - 1). Bei 48kHz und
    // einem Update pro Sample läuft ein uint64 erst nach ~12 Mio. Jahren über.
    // Getrennte Cache-Lines gegen False Sharing zwischen den Threads —
    // das von MSVC gemeldete Padding (C4324) ist hier gewollt.
    JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4324)
    alignas (64) std::atomic<juce::uint64> head { 0 };
    alignas (64) std::atomic<juce::uint64> tail { 0 };
    JUCE_END_IGNORE_WARNINGS_MSVC

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpscQueue)
};

} // namespace conduit
