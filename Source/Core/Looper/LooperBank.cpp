#include "LooperBank.h"

#include <cmath>

namespace conduit
{

LooperBank::LooperBank()
{
    // Vektor-Kapazität vorab (MT-Pfad — trotzdem keine Realloc-Kaskaden
    // beim Live-Hämmern von Commits)
    store.reserve (256);
    graveyard.reserve (256);
}

//==============================================================================
void LooperBank::prepare (double sampleRate)
{
    preparedSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    crossfadeSamples = juce::jmax (1, (int) std::lround (crossfadeSeconds * preparedSampleRate));
    maxLoopSamples   = (int) std::lround (maxLoopSeconds * preparedSampleRate);

    // Audio steht (prepareToPlay) — die SPSC-Rollen dürfen hier einmalig
    // vom MT übernommen werden: Queues leeren, Voices direkt zurücksetzen
    ClipCommand dropped;
    while (commands.pop (dropped)) {}

    LooperClip* released = nullptr;
    while (retired.pop (released))
        for (auto& grave : graveyard)
            if (grave.clip.get() == released)
                grave.audioReleased = true;

    for (auto& looper : players)
        for (auto& track : looper)
            for (auto& voice : track.voices)
                voice = Voice {};

    // Alle Clips verwerfen (SampleRate-Wechsel invalidiert Inhalte);
    // export-gepinnte bleiben im Graveyard, bis der Writer fertig ist
    for (auto& clip : store)
        if (clip->exportPins.load (std::memory_order_acquire) > 0)
            graveyard.push_back ({ std::move (clip), true });
    store.clear();

    std::erase_if (graveyard, [] (const Grave& grave)
    {
        return grave.clip->exportPins.load (std::memory_order_acquire) <= 0;
    });
    for (auto& grave : graveyard)
        grave.audioReleased = true;

    std::int64_t remaining = 0;
    for (const auto& grave : graveyard)
        remaining += grave.clip->allocatedBytes();
    ramBytesUsed.store (remaining, std::memory_order_relaxed);

    for (auto& looper : mtActiveClip)
        looper.fill (nullptr);

    playingFlag.store (false, std::memory_order_relaxed);
    committedBars.store (0, std::memory_order_relaxed);
    stopAllRequested.store (false, std::memory_order_relaxed);
    snapCount.store (0, std::memory_order_relaxed);

    playheadValid = false;
    snapPendingCount = 0;
    snapDucking = false;
    duckGain = 1.0f;
}

//==============================================================================
juce::Result LooperBank::commitAndPlay (int looperIndex, int trackIndex, int bars,
                                        const CaptureService& capture,
                                        int leftIndex, int rightIndex,
                                        const BarSampleAnchors& anchors)
{
    // Erst Quittungen einsammeln — gibt RAM frei, bevor das Budget prüft
    serviceMessageThread();

    if (preparedSampleRate <= 0.0)
        return juce::Result::fail ("Looper nicht vorbereitet");

    if (looperIndex < 0 || looperIndex >= maxLoopers
        || trackIndex < 0 || trackIndex >= maxTracks)
        return juce::Result::fail ("Ungültiger Looper/Track");

    if (leftIndex < 0)
        return juce::Result::fail ("Keine Looper-Quelle gewählt");

    const auto range = looper::commitRangeForBars (anchors.latestBoundaryBar(), bars);
    if (! range.valid)
        return juce::Result::fail ("Noch keine " + juce::String (bars)
                                   + " kompletten Takte in der Session");

    std::uint64_t startSample = 0, endSample = 0;
    if (! anchors.lookup (range.startBar, startSample)
        || ! anchors.lookup (range.endBar, endSample)
        || endSample <= startSample)
        return juce::Result::fail ("Taktgrenzen nicht mehr adressierbar");

    const auto numLoopSamples = endSample - startSample;
    if (numLoopSamples > static_cast<std::uint64_t> (maxLoopSamples))
        return juce::Result::fail ("Loop länger als "
                                   + juce::String ((int) maxLoopSeconds) + " s");

    const auto totalSamples = static_cast<int> (numLoopSamples) + crossfadeSamples;
    const auto newBytes = static_cast<std::int64_t> (totalSamples) * 2
                            * static_cast<std::int64_t> (sizeof (float))
                        + static_cast<std::int64_t> (sizeof (LooperClip));
    if (ramBytesUsed.load (std::memory_order_relaxed) + newBytes > ramBudgetBytes)
        return juce::Result::fail ("Looper-RAM-Budget erschöpft ("
                                   + juce::String (ramBudgetBytes / 1'000'000)
                                   + " MB) — Clips löschen");

    // Beide Kommandos (deleteClip + activate) müssen zusammen durchgehen
    if (commands.getNumFree() < 2)
        return juce::Result::fail ("Looper-Kommando-Queue voll");

    auto clip = std::make_unique<LooperClip>();
    clip->buffer.setSize (2, totalSamples);
    clip->buffer.clear();

    // Lead-in beginnt crossfadeSamples VOR dem Loop-Start; am Session-
    // Anfang fehlt er ggf. → der unlesbare Teil bleibt Stille (1:1 Engine)
    const auto leadIn = static_cast<std::uint64_t> (crossfadeSamples);
    const auto readStart = startSample >= leadIn ? startSample - leadIn : 0;
    const auto padMissing = static_cast<int> (leadIn - (startSample - readStart));

    const int indices[] = { leftIndex, rightIndex < 0 ? leftIndex : rightIndex };
    for (int channel = 0; channel < 2; ++channel)
        readChannelChunked (capture.getChannel (indices[channel]), readStart,
                            clip->buffer.getWritePointer (channel) + padMissing,
                            totalSamples - padMissing);

    clip->numContentSamples = static_cast<int> (numLoopSamples);
    clip->crossfadeSamples  = crossfadeSamples;
    clip->contentBeats = range.lengthBeats();
    clip->samplesPerBeatRecorded = static_cast<double> (numLoopSamples) / range.lengthBeats();
    clip->commitStartSample = startSample;
    clip->clipId = ++nextClipId;
    clip->commitBars = bars;

    clip->anchorBeat.store (range.endBeat(), std::memory_order_relaxed);
    clip->lengthBeats.store (range.lengthBeats(), std::memory_order_relaxed);
    clip->rate.store (1.0, std::memory_order_relaxed);
    clip->windowOffsetBeats.store (0.0, std::memory_order_relaxed);
    clip->reversed.store (false, std::memory_order_relaxed);

    auto* raw = clip.get();
    const auto l = static_cast<std::size_t> (looperIndex);
    const auto t = static_cast<std::size_t> (trackIndex);

    // Alten Clip des Tracks ersetzen: Graveyard + deleteClip VOR activate —
    // der Audio-Thread verarbeitet die Kommandos in Reihenfolge
    if (auto* old = mtActiveClip[l][t])
    {
        moveToGraveyard (old);
        commands.push ({ ClipCommand::Type::deleteClip, looperIndex, trackIndex, old });
    }

    store.push_back (std::move (clip));
    mtActiveClip[l][t] = raw;
    ramBytesUsed.fetch_add (newBytes, std::memory_order_relaxed);

    commands.push ({ ClipCommand::Type::activate, looperIndex, trackIndex, raw });

    committedBars.store (bars, std::memory_order_relaxed);
    playingFlag.store (true, std::memory_order_relaxed);
    return juce::Result::ok();
}

void LooperBank::moveToGraveyard (LooperClip* clip)
{
    for (auto& looper : mtActiveClip)
        for (auto& active : looper)
            if (active == clip)
                active = nullptr;

    for (auto it = store.begin(); it != store.end(); ++it)
    {
        if (it->get() == clip)
        {
            graveyard.push_back ({ std::move (*it), false });
            store.erase (it);
            return;
        }
    }

    jassertfalse;  // Clip war nicht im Store — Buchführungsfehler
}

void LooperBank::serviceMessageThread()
{
    LooperClip* released = nullptr;
    while (retired.pop (released))
    {
        bool found = false;
        for (auto& grave : graveyard)
        {
            if (grave.clip.get() == released)
            {
                grave.audioReleased = true;
                found = true;
                break;
            }
        }

        jassert (found);  // Quittung ohne Graveyard-Eintrag = Protokollfehler
        juce::ignoreUnused (found);
    }

    std::erase_if (graveyard, [this] (const Grave& grave)
    {
        if (! grave.audioReleased
            || grave.clip->exportPins.load (std::memory_order_acquire) > 0)
            return false;

        ramBytesUsed.fetch_sub (grave.clip->allocatedBytes(), std::memory_order_relaxed);
        return true;
    });
}

void LooperBank::readChannelChunked (const CaptureChannel* channel,
                                     std::uint64_t startPosition,
                                     float* dest, int numSamples)
{
    if (channel == nullptr || numSamples <= 0)
        return;

    // Export-Halte-Protokoll: parallel zum CaptureWriter legal (Zähler);
    // schlägt die Anmeldung fehl (Freigabe läuft), bleibt der Loop Stille
    auto* mutableChannel = const_cast<CaptureChannel*> (channel);
    if (! mutableChannel->tryBeginExportRead())
        return;

    // Chunked: ein Teilbereich mit Loch (Gate war zu) schlägt als Ganzes
    // fehl — kleinere Chunks begrenzen den Stille-Verschnitt an den Rändern
    constexpr int chunkSamples = 65536;
    for (int offset = 0; offset < numSamples; offset += chunkSamples)
    {
        const auto thisChunk = juce::jmin (chunkSamples, numSamples - offset);
        if (! channel->read (startPosition + static_cast<std::uint64_t> (offset),
                             dest + offset, thisChunk))
        {
            // Loch → Stille (buffer ist bereits geleert)
        }
    }

    mutableChannel->endExportRead();
}

//==============================================================================
void LooperBank::stopAll() noexcept
{
    stopAllRequested.store (true, std::memory_order_release);
    playingFlag.store (false, std::memory_order_relaxed);
}

//==============================================================================
bool LooperBank::anyVoiceReferences (const LooperClip* clip) const noexcept
{
    for (const auto& looper : players)
        for (const auto& track : looper)
            for (const auto& voice : track.voices)
                if (voice.clip == clip)
                    return true;

    return false;
}

bool LooperBank::transferOrRetire (LooperClip* clip) noexcept
{
    // Referenziert eine andere Voice den Clip noch (Retrigger), erbt SIE
    // die Retire-Pflicht; sonst geht die Quittung an den MT. false nur,
    // wenn die Retire-Queue voll ist (Caller parkt und versucht es im
    // nächsten Block erneut).
    for (auto& looper : players)
        for (auto& track : looper)
            for (auto& voice : track.voices)
                if (voice.clip == clip)
                {
                    voice.retireOnEnd = true;
                    return true;
                }

    return retired.push (clip);
}

void LooperBank::handleActivate (const ClipCommand& command) noexcept
{
    auto& player = players[static_cast<std::size_t> (command.looper)]
                          [static_cast<std::size_t> (command.track)];

    // Laufende Voices des Tracks ausblenden (Re-Commit-Crossfade)
    for (auto& voice : player.voices)
        if (voice.clip != nullptr)
            voice.fading = true;

    // Freie Voice nehmen; sind alle belegt (Doppel-Re-Commit im Fade-
    // Fenster), fällt die leiseste — deren Retire-Pflicht wandert sofort
    // in die Queue (Drain-Guard hat den Platz reserviert)
    Voice* slot = nullptr;
    for (auto& voice : player.voices)
    {
        if (voice.clip == nullptr)
        {
            slot = &voice;
            break;
        }

        if (slot == nullptr || voice.gain < slot->gain)
            slot = &voice;
    }

    if (slot->clip != nullptr && slot->retireOnEnd)
    {
        // Gestohlene Voice trug die Retire-Pflicht → Pflicht übergeben
        // bzw. sofort quittieren (Drain-Guard hat den Queue-Platz reserviert)
        auto* stolen = slot->clip;
        slot->clip = nullptr;
        transferOrRetire (stolen);
    }

    slot->clip = command.clip;
    slot->gain = 0.0f;
    slot->fading = false;
    slot->retireOnEnd = false;
}

void LooperBank::handleDelete (const ClipCommand& command) noexcept
{
    // Über die ganze Matrix suchen (Retrigger kann denselben Clip in
    // mehreren Voices halten); genau EINE Voice erbt die Retire-Pflicht
    bool owner = false;
    for (auto& looper : players)
        for (auto& track : looper)
            for (auto& voice : track.voices)
            {
                if (voice.clip != command.clip)
                    continue;

                voice.fading = true;
                if (! owner)
                {
                    voice.retireOnEnd = true;
                    owner = true;
                }
            }

    if (! owner)
        retired.push (command.clip);  // nirgends referenziert → sofort quittieren
}

void LooperBank::drainCommands() noexcept
{
    // Drain-Guard: nur konsumieren, solange die Retire-Queue Luft für alle
    // denkbaren Quittungen hat — Überschuss wartet einen Block
    const auto guard = maxLoopers * maxTracks * voicesPerTrack + 2;

    ClipCommand command;
    while (retired.getNumFree() > guard && commands.pop (command))
    {
        if (command.looper < 0 || command.looper >= maxLoopers
            || command.track < 0 || command.track >= maxTracks)
            continue;

        switch (command.type)
        {
            case ClipCommand::Type::activate:   handleActivate (command); break;
            case ClipCommand::Type::deleteClip: handleDelete (command); break;

            case ClipCommand::Type::stopTrack:
                for (auto& voice : players[static_cast<std::size_t> (command.looper)]
                                          [static_cast<std::size_t> (command.track)].voices)
                    if (voice.clip != nullptr)
                        voice.fading = true;
                break;
        }
    }
}

//==============================================================================
float LooperBank::renderClipSample (const LooperClip& clip, int channel,
                                    double contentPosition) noexcept
{
    // Buffer-Layout: [0, F) Lead-in, [F, F+N) Content — Position ∈ [0, N)
    const auto* data = clip.buffer.getReadPointer (channel);
    const auto fade = clip.crossfadeSamples;
    const auto lastIndex = clip.numContentSamples + fade - 1;

    const auto readLinear = [&] (double position) noexcept -> float
    {
        const auto clamped = juce::jlimit (0.0, static_cast<double> (lastIndex), position);
        const auto index = static_cast<int> (clamped);
        const auto frac = static_cast<float> (clamped - index);
        const auto next = juce::jmin (index + 1, lastIndex);
        return data[index] + (data[next] - data[index]) * frac;
    };

    const auto zoneStart = static_cast<double> (clip.numContentSamples - fade);

    // Außerhalb der Wrap-Zone: direkter Content-Read
    if (contentPosition < zoneStart || clip.numContentSamples <= fade)
        return readLinear (static_cast<double> (fade) + contentPosition);

    // Wrap-Zone [N−F, N): equal-power vom Content-Ende auf den Lead-in —
    // bei alpha → 1 landet die Blende exakt auf dem Loop-Start-Sample
    const auto zonePosition = contentPosition - zoneStart;          // [0, F)
    const auto alpha = zonePosition / static_cast<double> (fade);   // 0..1

    const auto endSample  = readLinear (static_cast<double> (fade) + contentPosition);
    const auto leadSample = readLinear (zonePosition);              // Lead-in → Loop-Start

    const auto angle = alpha * juce::MathConstants<double>::halfPi;
    return endSample  * static_cast<float> (std::cos (angle))
         + leadSample * static_cast<float> (std::sin (angle));
}

void LooperBank::process (juce::AudioBuffer<float>& buffer, int numOutputChannels,
                          const ClockState& clock, std::uint64_t blockStartSample,
                          const BarSampleAnchors& anchors) noexcept
{
    // Stop: alle Voices ausblenden (Clips bleiben im Store)
    if (stopAllRequested.exchange (false, std::memory_order_acq_rel))
        for (auto& looper : players)
            for (auto& track : looper)
                for (auto& voice : track.voices)
                    if (voice.clip != nullptr)
                        voice.fading = true;

    drainCommands();

    const auto beatsPerSample = clock.beatsPerSample();
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || beatsPerSample <= 0.0)
        return;

    // Beat-Messung jitter-frei aus der SampleClock (1:1 LooperEngine —
    // Herleitung dort: Anker-Beat + Sample-Abstand statt Wall-Clock)
    auto measuredBeat = clock.beatAtBlockStart;
    {
        const auto latestBar = anchors.latestBoundaryBar();
        std::uint64_t anchorSample = 0;
        if (latestBar >= 0 && anchors.lookup (latestBar, anchorSample))
            measuredBeat = static_cast<double> (latestBar) * looper::quantumBeats
                         + static_cast<double> (static_cast<std::int64_t> (
                               blockStartSample - anchorSample))
                               * beatsPerSample;
    }

    if (! playheadValid)
    {
        playheadBeat  = measuredBeat;
        playheadValid = true;
        snapPendingCount = 0;
        snapDucking = false;
        duckGain = 1.0f;
    }
    else if (std::abs (measuredBeat - playheadBeat) > snapThresholdBeats)
    {
        // Snap-Kandidat: erst nach snapConfirmBlocks Blöcken glauben
        if (++snapPendingCount >= snapConfirmBlocks)
            snapDucking = true;
    }
    else
    {
        snapPendingCount = 0;
    }

    // Duck bei Stille angekommen → Playhead UNTER der Stille umsetzen
    if (snapDucking && duckGain <= 0.0f)
    {
        playheadBeat = measuredBeat;
        snapDucking = false;
        snapPendingCount = 0;
        snapCount.fetch_add (1, std::memory_order_relaxed);
    }

    // Slew-limitierte Korrektur (unhörbares Varispeed, 1:1 LooperEngine)
    const auto maxCorrection = maxSlewRatio * beatsPerSample
                             * static_cast<double> (numSamples);
    const auto correction = juce::jlimit (-maxCorrection, maxCorrection,
                                          measuredBeat - playheadBeat);
    const auto beatStep = beatsPerSample
                        + correction / static_cast<double> (numSamples);
    const auto blockStartBeat = playheadBeat;
    playheadBeat += beatStep * static_cast<double> (numSamples);

    const auto pair = anchor.load (std::memory_order_relaxed);
    const auto channelA = pair * 2;
    const auto usableChannels = juce::jmin (numOutputChannels, buffer.getNumChannels());
    const auto writable = channelA < usableChannels;

    auto* outA = writable ? buffer.getWritePointer (channelA) : nullptr;
    auto* outB = (writable && channelA + 1 < usableChannels)
               ? buffer.getWritePointer (channelA + 1) : nullptr;

    const auto gainStep = 1.0f / static_cast<float> (juce::jmax (1, crossfadeSamples));

    // Duck-Rampe des Blocks (Snap-Declick): linear innerhalb des Blocks
    const auto duckStartGain = duckGain;
    const auto duckStep = (snapDucking ? -1.0f : 1.0f) * gainStep;
    duckGain = juce::jlimit (0.0f, 1.0f,
                             duckGain + duckStep * static_cast<float> (numSamples));

    for (auto& looper : players)
        for (auto& track : looper)
            for (auto& voice : track.voices)
            {
                auto* clip = voice.clip;
                if (clip == nullptr)
                    continue;

                if (clip->numContentSamples <= 0 || clip->contentBeats <= 0.0)
                {
                    voice.clip = nullptr;
                    continue;
                }

                // Clip-Parameter blockkonstant lesen (MT schreibt Atomics)
                const auto anchorB = clip->anchorBeat.load (std::memory_order_relaxed);
                const auto lengthB = clip->lengthBeats.load (std::memory_order_relaxed);
                const auto rateV   = clip->rate.load (std::memory_order_relaxed);
                const auto revV    = clip->reversed.load (std::memory_order_relaxed);
                const auto offsetB = clip->windowOffsetBeats.load (std::memory_order_relaxed);
                const auto maxPosition = static_cast<double> (clip->numContentSamples) - 1.0e-9;

                if (lengthB <= 0.0)
                {
                    voice.clip = nullptr;
                    continue;
                }

                const auto fading = voice.fading;

                for (int i = 0; i < numSamples; ++i)
                {
                    const auto beat = blockStartBeat + beatStep * i;
                    const auto position = juce::jlimit (
                        0.0, maxPosition,
                        looper::clipReadPosition (beat, anchorB, rateV, lengthB,
                                                  revV, offsetB,
                                                  clip->samplesPerBeatRecorded));

                    voice.gain = fading ? juce::jmax (0.0f, voice.gain - gainStep)
                                        : juce::jmin (1.0f, voice.gain + gainStep);

                    if (voice.gain <= 0.0f && fading)
                        break;

                    // Anker außerhalb der Kanalzahl: Fades trotzdem
                    // nachführen (Gerätewechsel hinterlässt keine Zombies)
                    if (outA == nullptr)
                        continue;

                    const auto duck = juce::jlimit (0.0f, 1.0f,
                                                    duckStartGain + duckStep * static_cast<float> (i));

                    outA[i] += renderClipSample (*clip, 0, position) * voice.gain * duck;
                    if (outB != nullptr)
                        outB[i] += renderClipSample (*clip, 1, position) * voice.gain * duck;
                }

                // Voice-Ende: Retire-Pflicht quittieren, sobald keine andere
                // Voice den Clip mehr referenziert (Retrigger-Schutz);
                // schlägt der Push fehl, parkt die Voice bis zum nächsten Block
                if (fading && voice.gain <= 0.0f)
                {
                    if (voice.retireOnEnd)
                    {
                        auto* toRetire = voice.clip;
                        voice.clip = nullptr;

                        if (! transferOrRetire (toRetire))
                        {
                            voice.clip = toRetire;      // Queue voll → nächster Block
                            continue;
                        }
                    }
                    else
                    {
                        voice.clip = nullptr;
                    }

                    voice.gain = 0.0f;
                    voice.fading = false;
                    voice.retireOnEnd = false;
                }
            }
}

} // namespace conduit
