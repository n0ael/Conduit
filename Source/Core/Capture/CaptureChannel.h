#pragma once

#include <atomic>
#include <cstdint>

#include <juce_core/juce_core.h>

#include "BufferPool.h"
#include "CaptureRingBuffer.h"
#include "PreRollBuffer.h"

namespace conduit
{

//==============================================================================
/**
    Aufnahme-Zustandsmaschine eines Capture-Kanals.

    Zustände (atomar publiziert):
      idle            — Gate zu, kein Speicher gebunden
      awaitingSegment — Gate offen, Segment beim BufferPool angefordert;
                        der PreRollBuffer überbrückt die Pool-Latenz
      recording       — Segment gebunden, Live-Input wird geschrieben
      held            — Gate wieder zu, Material bleibt gebunden, bis der
                        RAM-Wächter freigibt (Hold-Zeit und Export folgen
                        mit Capture-Baustein 4/5)

    Pre-Roll-Übernahme (amortisiert, allocation-free):
    Beim Gate-Open gilt startSamplePosition = SampleClock − preRollLength,
    geclamped auf den gültigen Pre-Roll-Bestand — die Pool-Latenz knabbert
    also höchstens Millisekunden am ÄLTESTEN Rand, nie an den Live-Daten.
    Sobald das Segment da ist, kopiert process() pro Block höchstens
    budget Samples aus dem Pre-Roll in den Ring, bis die Lücke
    [takeoverCursor, takeoverEnd) geschlossen ist. Das Budget muss
    ≥ 2× Blockgröße sein, sonst könnte der weiterlaufende Pre-Roll-Ring
    noch unkopierte Samples überschreiben (Herleitung und Callback-Budget
    in CaptureService::buildSet). Live-Samples landen parallel direkt an
    ihrer absoluten Position — der Ring ist positionsadressiert, die
    Speicher-Reihenfolge ergibt sich von selbst.

    Wiedereröffnung aus held: liegt die Gate-Pause noch im Pre-Roll-Fenster,
    wird nahtlos fortgesetzt (die Pause kommt rückwirkend aus dem Pre-Roll
    in den Ring); sonst wird neu geankert und das alte Material verworfen —
    vorläufig, bis Baustein 5 beim Gate-Close exportiert.

    Threading: openGate/closeGate/process/releaseStorage NUR Audio Thread.
    requestRelease() [Message Thread, RAM-Wächter] setzt ein atomares Flag,
    das der Audio Thread im nächsten Block quittiert (nur im Zustand held).
    Status-Getter sind atomare Momentaufnahmen von jedem Thread. read()
    folgt der Leser-Disziplin des CaptureRingBuffer: hinter dem
    Schreib-Cursor, kein paralleles attach/detach (Baustein 5 definiert
    das Halte-Protokoll des Exports).
*/
class CaptureChannel
{
public:
    enum class State : int { idle, awaitingSegment, recording, held };

    /** Zusammenhängend gültiger Bereich [from, to) in absoluten Positionen. */
    struct ReadableRange
    {
        std::uint64_t from = 0;
        std::uint64_t to   = 0;
    };

    CaptureChannel() = default;

    /** [Message Thread, Set noch nicht publiziert] */
    void prepare (PreRollBuffer& preRollToUse, BufferPool& poolToUse,
                  int takeoverBudgetSamples);

    //==========================================================================
    // [Audio Thread] — Baustein 4 (Signal-Detektion) ruft die Gates später

    void openGate (std::uint64_t clockNow, int preRollWindowSamples) noexcept;
    void closeGate (std::uint64_t clockNow) noexcept;

    /** Pro Block, VOR dem PreRollBuffer::write() desselben Blocks
        (die Übernahme muss die ältesten Ring-Inhalte lesen, bevor der
        neue Block sie verdrängt — Reihenfolge stellt CaptureService sicher). */
    void process (const float* input, int numSamples, std::uint64_t blockStart) noexcept;

    /** Segment an den Pool zurückgeben (Invalidate / Release-Quittung). */
    void releaseStorage() noexcept;

    //==========================================================================
    /** [Message Thread] RAM-Wächter: Freigabe anfordern — der Audio Thread
        quittiert im nächsten Block, ausschließlich im Zustand held. */
    void requestRelease() noexcept
    {
        releaseRequested.store (true, std::memory_order_release);
    }

    //==========================================================================
    // Status [beliebiger Thread] — atomare Momentaufnahmen

    [[nodiscard]] State getState() const noexcept
    {
        return state.load (std::memory_order_acquire);
    }

    [[nodiscard]] bool isTakeoverComplete() const noexcept
    {
        return takeoverCursor.load (std::memory_order_acquire)
               >= takeoverEnd.load (std::memory_order_relaxed);
    }

    /** SampleClock-Stand des ersten Samples im Ring (Anker). */
    [[nodiscard]] std::uint64_t getStartSamplePosition() const noexcept
    {
        return ring.getStartSamplePosition();
    }

    /** Absolute Position nach dem letzten publizierten Live-Sample. */
    [[nodiscard]] std::uint64_t getEndPosition() const noexcept
    {
        return ring.getEndPosition();
    }

    /** SampleClock-Stand des Gate-Close (nur im Zustand held aussagekräftig)
        — der RAM-Wächter gibt den ältesten gehaltenen Kanal zuerst frei. */
    [[nodiscard]] std::uint64_t getHeldSincePosition() const noexcept
    {
        return heldSince.load (std::memory_order_relaxed);
    }

    [[nodiscard]] ReadableRange getReadableRange() const noexcept;

    /** [Leser hinter dem Schreib-Cursor — Baustein 5 / Tests]
        false, wenn [position, position + numSamples) nicht vollständig im
        zusammenhängend gültigen Bereich liegt. */
    bool read (std::uint64_t position, float* dest, int numSamples) const noexcept;

private:
    void beginTakeover (float* claimedSegment, std::uint64_t blockStart) noexcept;
    void reopenFromHeld (std::uint64_t clockNow, int newWindowSamples) noexcept;
    void copyTakeoverChunk (int budgetSamples) noexcept;

    PreRollBuffer* preRoll = nullptr;
    BufferPool*    pool    = nullptr;
    CaptureRingBuffer ring;
    float* segment = nullptr;

    // Nur Audio Thread
    int takeoverBudget = 0;
    int windowSamples = 0;
    std::uint64_t gateOpenPosition = 0;
    std::uint64_t takeoverCursorLocal = 0;
    std::uint64_t takeoverEndLocal = 0;
    std::uint64_t lastLiveEnd = 0;  // Aussetzer-Erkennung im Live-Strom

    // Publiziert [Audio → beliebiger Thread]
    std::atomic<State> state { State::idle };
    std::atomic<bool> releaseRequested { false };
    std::atomic<std::uint64_t> heldSince { 0 };
    std::atomic<std::uint64_t> takeoverCursor { 0 };
    std::atomic<std::uint64_t> takeoverEnd { 0 };
    std::atomic<std::uint64_t> contiguousFrom { 0 };  // Lücken-Schutz (siehe .cpp)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureChannel)
};

} // namespace conduit
