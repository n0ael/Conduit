// Link/asio zuerst — die Netzwerk-Header (WinSock2 etc.) müssen vor allem
// stehen, was windows.h einziehen könnte (JUCE). LinkAudio.hpp ERSETZT
// Link.hpp (CLAUDE.md 7.2) und muss in jeder Compilation Unit stehen, die
// LinkAudio-Typen berührt — die Link-Header sind nicht selbsttragend.
#include <ableton/LinkAudio.hpp>

#include <algorithm>
#include <optional>

#include "LinkClock.h"

namespace conduit
{

struct LinkClock::Impl
{
    Impl (double initialBpm, const juce::String& peerName)
        : link (initialBpm, peerName.toStdString())
    {
        link.enableStartStopSync (true);
        link.enable (true);

        // Audio-Sharing aktiviert erst das erste Send-Modul (7.2) —
        // bis dahin verhält sich LinkAudio exakt wie Link.
        link.enableLinkAudio (false);
    }

    ableton::LinkAudio link;

    // SessionState des aktuellen Blocks — captureClockState() schreibt,
    // Sink::commitFromClockState() liest im SELBEN Audio-Callback (der
    // EngineProcessor füllt den ClockBus vor dem Graph-Render). Bewusst
    // kein Atomic: Audio-Thread-only. So nutzt commit() exakt die
    // SessionState/Beat/Quantum-Basis des lokalen Renderings (7.2), ohne
    // dass Module ein zweites captureAudioSessionState brauchen.
    std::optional<ableton::Link::SessionState> blockSessionState;
};

struct LinkClock::Sink::Impl
{
    Impl (LinkClock::Impl& ownerImpl, const juce::String& name, std::size_t maxNumSamples)
        : owner (ownerImpl), sink (ownerImpl.link, name.toStdString(), maxNumSamples)
    {
    }

    LinkClock::Impl& owner;  // Sink darf die LinkClock nicht überleben (Header-Doku)
    ableton::LinkAudioSink sink;
};

//==============================================================================
LinkClock::LinkClock (double initialBpm, const juce::String& peerName)
    : impl (std::make_unique<Impl> (initialBpm, peerName))
{
    // ChannelsChangedCallback kommt auf einem Link-Thread (7.2) — Marshalling
    // auf den Message Thread; WeakReference deckt den Fall ab, dass die
    // LinkClock vor Zustellung des async Calls destruiert wird.
    impl->link.setChannelsChangedCallback (
        [weakThis = juce::WeakReference<LinkClock> (this)]
        {
            juce::MessageManager::callAsync ([weakThis]
            {
                if (auto* clock = weakThis.get())
                    clock->sendChangeMessage();
            });
        });
}

LinkClock::~LinkClock() = default;

void LinkClock::prepare (double sampleRate) noexcept
{
    currentSampleRate.store (sampleRate, std::memory_order_relaxed);
}

//==============================================================================
void LinkClock::setTempo (double bpm)
{
    auto sessionState = impl->link.captureAppSessionState();
    sessionState.setTempo (bpm, impl->link.clock().micros());
    impl->link.commitAppSessionState (sessionState);
}

double LinkClock::getTempo() const
{
    return impl->link.captureAppSessionState().tempo();
}

std::size_t LinkClock::getNumPeers() const
{
    return impl->link.numPeers();
}

//==============================================================================
void LinkClock::enableAudio (bool shouldBeEnabled)
{
    audioRefCount += shouldBeEnabled ? 1 : -1;
    jassert (audioRefCount >= 0);  // unbalancierter enableAudio(false)-Aufruf
    audioRefCount = juce::jmax (audioRefCount, 0);

    impl->link.enableLinkAudio (audioRefCount > 0);
}

bool LinkClock::isAudioEnabled() const
{
    return impl->link.isLinkAudioEnabled();
}

juce::String LinkClock::peerName() const
{
    return juce::String (impl->link.peerName());
}

void LinkClock::setPeerName (const juce::String& name)
{
    impl->link.setPeerName (name.toStdString());
}

//==============================================================================
LinkClock::Sink::Sink (std::unique_ptr<Impl> implToUse)
    : impl (std::move (implToUse))
{
}

LinkClock::Sink::~Sink() = default;

juce::String LinkClock::Sink::getName() const
{
    return juce::String (impl->sink.name());
}

void LinkClock::Sink::setName (const juce::String& newName)
{
    impl->sink.setName (newName.toStdString());
}

std::size_t LinkClock::Sink::getMaxNumSamples() const noexcept
{
    return impl->sink.maxNumSamples();
}

void LinkClock::Sink::requestMaxNumSamples (std::size_t numSamples) noexcept
{
    // Die Link-Header-Doku verspricht No-op beim Schrumpfen, die
    // Implementierung (link_audio::Sink) weist aber bedingungslos zu —
    // wir erzwingen die dokumentierte Semantik selbst, damit ein
    // Re-Prepare mit kleinerem Block keine Kapazität verliert.
    if (numSamples > impl->sink.maxNumSamples())
        impl->sink.requestMaxNumSamples (numSamples);
}

LinkClock::Sink::CommitResult LinkClock::Sink::commitFromClockState (
    const std::int16_t* interleavedSamples, int numFrames, int numChannels,
    const ClockState& clock) noexcept
{
    // Audio Thread, RT-safe (Link-Garantie für BufferHandle + commit).
    if (! impl->owner.blockSessionState.has_value()
        || interleavedSamples == nullptr || numFrames <= 0 || numChannels <= 0)
        return CommitResult::rejected;

    ableton::LinkAudioSink::BufferHandle handle (impl->sink);

    if (! handle)
        return CommitResult::noBuffer;  // kein Subscriber oder Queue voll

    const auto numSamples = static_cast<std::size_t> (numFrames)
                          * static_cast<std::size_t> (numChannels);

    // SAMPLES, nicht Frames (7.2) — größer als die Sink-Kapazität wäre die
    // v1-Verwechslung. Das Handle released seinen Buffer im Destruktor.
    if (numSamples > handle.maxNumSamples)
        return CommitResult::rejected;

    std::copy_n (interleavedSamples, numSamples, handle.samples);

    return handle.commit (*impl->owner.blockSessionState,
                          clock.beatAtBlockStart,
                          quantum,
                          static_cast<std::size_t> (numFrames),
                          static_cast<std::size_t> (numChannels),
                          static_cast<std::uint32_t> (clock.sampleRate))
               ? CommitResult::committed
               : CommitResult::rejected;
}

std::unique_ptr<LinkClock::Sink> LinkClock::createSink (const juce::String& name,
                                                        std::size_t maxNumSamples)
{
    // new statt make_unique: der Sink-Ctor ist privat (friend LinkClock),
    // das Ergebnis landet unmittelbar im unique_ptr.
    return std::unique_ptr<Sink> (
        new Sink (std::make_unique<Sink::Impl> (*impl, name, maxNumSamples)));
}

//==============================================================================
ClockState LinkClock::captureClockState (int) noexcept
{
    // Lock-free (Link-Garantie für die capture/commitAudioSessionState-API).
    // numSamples bleibt für spätere Output-Latenz-Kompensation reserviert.
    const auto sessionState = impl->link.captureAudioSessionState();
    const auto now = impl->link.clock().micros();

    // Stash für Sink::commitFromClockState im selben Callback (7.2) —
    // optional mit Inline-Storage, die Zuweisung allokiert nicht.
    impl->blockSessionState = sessionState;

    ClockState state;
    state.bpm              = sessionState.tempo();
    state.beatAtBlockStart = sessionState.beatAtTime (now, quantum);
    state.sampleRate       = currentSampleRate.load (std::memory_order_relaxed);
    state.isPlaying        = sessionState.isPlaying();
    return state;
}

} // namespace conduit
