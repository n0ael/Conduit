#include "CaptureChannel.h"

namespace conduit
{

void CaptureChannel::prepare (PreRollBuffer& preRollToUse, BufferPool& poolToUse,
                              int takeoverBudgetSamples)
{
    preRoll        = &preRollToUse;
    pool           = &poolToUse;
    takeoverBudget = juce::jmax (0, takeoverBudgetSamples);
}

//==============================================================================
void CaptureChannel::openGate (std::uint64_t clockNow, int preRollWindowSamples) noexcept
{
    if (detachPending)
        return;  // Kanal stillgelegt, bis die aufgeschobene Freigabe durch ist

    switch (state.load (std::memory_order_relaxed))
    {
        case State::idle:
            gateOpenPosition = clockNow;
            windowSamples    = juce::jmax (0, preRollWindowSamples);
            if (pool != nullptr)
                pool->requestSegment();
            state.store (State::awaitingSegment, std::memory_order_release);
            break;

        case State::held:
            reopenFromHeld (clockNow, juce::jmax (0, preRollWindowSamples));
            break;

        case State::awaitingSegment:
        case State::recording:
            break;  // Gate bereits offen
    }
}

void CaptureChannel::closeGate (std::uint64_t clockNow) noexcept
{
    switch (state.load (std::memory_order_relaxed))
    {
        case State::recording:
            heldSince.store (clockNow, std::memory_order_relaxed);
            state.store (State::held, std::memory_order_release);
            break;

        case State::awaitingSegment:
            // Nichts gebunden — die Pool-Anforderung bleibt stehen, das
            // Surplus-Segment bedient das nächste Gate-Open sofort
            state.store (State::idle, std::memory_order_release);
            break;

        case State::idle:
        case State::held:
            break;
    }
}

//==============================================================================
void CaptureChannel::process (const float* input, int numSamples,
                              std::uint64_t blockStart) noexcept
{
    // RAM-Wächter-Quittung: Freigabe nur für gehaltenes (inaktives) Material
    if (releaseRequested.exchange (false, std::memory_order_acq_rel))
        if (state.load (std::memory_order_relaxed) == State::held)
            releaseStorage();  // kann aufschieben (Export-Halte-Protokoll)

    // Aufgeschobene Freigabe nachholen, sobald der letzte Export-Leser weg
    // ist; bis dahin ist der Kanal stillgelegt (Klassendoku)
    if (detachPending)
    {
        if (exportReaders.load (std::memory_order_seq_cst) == 0)
            releaseStorage();

        if (detachPending)
            return;
    }

    if (state.load (std::memory_order_relaxed) == State::awaitingSegment && pool != nullptr)
        if (auto* claimed = pool->tryClaimSegment())
            beginTakeover (claimed, blockStart);

    // Amortisierte Pre-Roll-Übernahme — läuft auch im Zustand held weiter,
    // damit gehaltenes Material vollständig wird. Muss VOR dem
    // PreRollBuffer::write() dieses Blocks laufen, sonst könnten die
    // ältesten noch unkopierten Samples gerade verdrängt werden.
    if (ring.hasStorage() && takeoverCursorLocal < takeoverEndLocal)
        copyTakeoverChunk (juce::jmax (takeoverBudget, 2 * numSamples));

    if (state.load (std::memory_order_relaxed) == State::recording
        && input != nullptr && numSamples > 0)
    {
        if (blockStart != lastLiveEnd)
        {
            // Aussetzer im Live-Strom: ein Loch im Ring darf nie als gültig
            // ausgeliefert werden — zusammenhängend ist nur noch ab hier
            contiguousFrom.store (blockStart, std::memory_order_relaxed);
            takeoverCursorLocal = takeoverEndLocal;  // Rest-Übernahme zwecklos
            takeoverCursor.store (takeoverCursorLocal, std::memory_order_release);
        }

        ring.writeAt (blockStart, input, numSamples);
        const auto liveEnd = blockStart + static_cast<std::uint64_t> (numSamples);
        lastLiveEnd = liveEnd;
        ring.publishEnd (liveEnd);
    }
}

void CaptureChannel::releaseStorage() noexcept
{
    // Dekker-Paar mit tryBeginExportRead (beidseitig seq_cst): erst die
    // Barriere setzen, DANN Leser zählen — entweder sieht ein einsteigender
    // Leser die Barriere (und zieht zurück), oder wir sehen ihn hier und
    // schieben die Freigabe auf. detach() bei aktivem Leser wäre eine
    // Data Race auf dem Storage-Pointer.
    releaseBarrier.store (true, std::memory_order_seq_cst);
    if (exportReaders.load (std::memory_order_seq_cst) > 0)
    {
        detachPending = true;  // process() holt die Freigabe nach
        return;
    }

    if (segment != nullptr)
    {
        ring.detach();
        if (pool != nullptr)
            pool->releaseSegment (segment);
        segment = nullptr;
    }

    takeoverCursorLocal = 0;
    takeoverEndLocal    = 0;
    lastLiveEnd         = 0;
    detachPending       = false;
    takeoverCursor.store (0, std::memory_order_relaxed);
    takeoverEnd.store (0, std::memory_order_relaxed);
    contiguousFrom.store (0, std::memory_order_relaxed);
    state.store (State::idle, std::memory_order_release);
    releaseBarrier.store (false, std::memory_order_seq_cst);
}

//==============================================================================
void CaptureChannel::beginTakeover (float* claimedSegment, std::uint64_t blockStart) noexcept
{
    segment = claimedSegment;

    const auto window  = static_cast<std::uint64_t> (windowSamples);
    const auto desired = gateOpenPosition > window ? gateOpenPosition - window : 0;

    // Pool-Latenz: der Pre-Roll lief während der Wartezeit weiter — älter
    // als sein gültiger Bestand geht nicht (knabbert ms am ältesten Rand)
    auto copyStart = juce::jmax (desired, preRoll->getOldestValidPosition());
    copyStart      = juce::jmin (copyStart, blockStart);  // defensiv

    ring.attach (segment, pool->getSegmentSamples(), copyStart);
    contiguousFrom.store (copyStart, std::memory_order_relaxed);

    takeoverCursorLocal = copyStart;
    takeoverEndLocal    = blockStart;
    lastLiveEnd         = blockStart;
    takeoverEnd.store (blockStart, std::memory_order_relaxed);
    takeoverCursor.store (copyStart, std::memory_order_release);

    state.store (State::recording, std::memory_order_release);
}

void CaptureChannel::reopenFromHeld (std::uint64_t clockNow, int newWindowSamples) noexcept
{
    windowSamples    = newWindowSamples;
    gateOpenPosition = clockNow;

    const auto window  = static_cast<std::uint64_t> (windowSamples);
    const auto desired = clockNow > window ? clockNow - window : 0;
    const auto liveEnd = lastLiveEnd;  // Ring-Ende der gehaltenen Aufnahme
    const auto oldest  = preRoll->getOldestValidPosition();

    if (liveEnd >= desired && liveEnd >= oldest
        && takeoverCursorLocal >= takeoverEndLocal)
    {
        // Nahtlose Fortsetzung: die Gate-Pause [liveEnd, clockNow) liegt
        // noch im Pre-Roll — rückwirkend auffüllen, Anker und Material bleiben
        takeoverCursorLocal = liveEnd;
        takeoverEndLocal    = clockNow;
    }
    else
    {
        // Pause länger als das Pre-Roll-Fenster (oder alte Übernahme nie
        // fertig geworden): neu ankern, das alte Material verfällt.
        // reanchor statt attach: Storage bleibt derselbe, nur die
        // Positions-Atomics wandern — ein parallel laufender Export-Leser
        // sieht die Verschiebung und bricht sauber ab (read() validiert nach)
        auto copyStart = juce::jmax (desired, oldest);
        copyStart      = juce::jmin (copyStart, clockNow);

        ring.reanchor (copyStart);
        contiguousFrom.store (copyStart, std::memory_order_relaxed);
        takeoverCursorLocal = copyStart;
        takeoverEndLocal    = clockNow;
    }

    lastLiveEnd = clockNow;
    takeoverEnd.store (takeoverEndLocal, std::memory_order_relaxed);
    takeoverCursor.store (takeoverCursorLocal, std::memory_order_release);
    state.store (State::recording, std::memory_order_release);
}

void CaptureChannel::copyTakeoverChunk (int budgetSamples) noexcept
{
    auto cursor       = takeoverCursorLocal;
    const auto target = takeoverEndLocal;
    const auto oldest = preRoll->getOldestValidPosition();

    if (cursor < oldest)
    {
        // Budget-Verletzung — mit Budget ≥ 2× Blockgröße und der
        // Kopiere-vor-Schreiben-Reihenfolge nie erreichbar. Defensiv: den
        // verlorenen Bereich aus der zusammenhängenden Auslieferung nehmen
        jassertfalse;
        contiguousFrom.store (oldest, std::memory_order_relaxed);
        cursor = juce::jmin (oldest, target);
    }

    const auto pending = target - cursor;
    const auto todo = static_cast<int> (
        juce::jmin (pending, static_cast<std::uint64_t> (juce::jmax (0, budgetSamples))));

    if (todo > 0)
    {
        ring.writeSpansAt (cursor, preRoll->makeReadSpans (cursor, todo));
        cursor += static_cast<std::uint64_t> (todo);
    }

    takeoverCursorLocal = cursor;
    takeoverCursor.store (cursor, std::memory_order_release);
}

//==============================================================================
CaptureChannel::ReadableRange CaptureChannel::getReadableRange() const noexcept
{
    ReadableRange range;

    const auto currentState = state.load (std::memory_order_acquire);
    if (currentState == State::idle || currentState == State::awaitingSegment)
        return range;

    const auto cursor = takeoverCursor.load (std::memory_order_acquire);
    const auto target = takeoverEnd.load (std::memory_order_relaxed);
    const auto end    = ring.getEndPosition();
    const auto start  = ring.getStartSamplePosition();

    auto from = juce::jmax (start, contiguousFrom.load (std::memory_order_relaxed));

    // Übernahme unvollständig: zusammenhängend nur bis zum Kopier-Cursor —
    // die Live-Daten dahinter werden erst mit geschlossener Lücke gültig
    auto to = cursor < target ? cursor : end;

    // Wrap-Verdrängung: der Ring hält höchstens capacity Samples
    const auto capacity = static_cast<std::uint64_t> (
        pool != nullptr ? pool->getSegmentSamples() : 0);
    if (capacity > 0 && to > from && to - from > capacity)
        from = to - capacity;

    if (to < from)
        to = from;

    range.from = from;
    range.to   = to;
    return range;
}

bool CaptureChannel::read (std::uint64_t position, float* dest, int numSamples) const noexcept
{
    if (dest == nullptr || numSamples < 0)
        return false;

    const auto range = getReadableRange();
    if (position < range.from
        || position + static_cast<std::uint64_t> (numSamples) > range.to)
        return false;

    ring.copyOut (position, dest, numSamples);

    // Nachvalidierung (Export-Halte-Protokoll): ist die ReadableRange
    // während des Kopierens per Wrap oder Re-Anker über den gelesenen
    // Bereich gewandert, ist das Ergebnis unbrauchbar. Der Überholschutz
    // des CaptureWriter hält den Leser ohnehin auf Abstand — das hier ist
    // die letzte Verteidigungslinie.
    const auto after = getReadableRange();
    return position >= after.from && after.to >= range.to;
}

} // namespace conduit
