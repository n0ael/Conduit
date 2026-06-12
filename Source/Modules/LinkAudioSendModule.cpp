#include "LinkAudioSendModule.h"

namespace conduit
{

LinkAudioSendModule::LinkAudioSendModule()
    : NetworkIOModule (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

LinkAudioSendModule::~LinkAudioSendModule()
{
    cancelPendingUpdate();

    // Der Destruktor läuft erst, wenn der Graph den Node und jede
    // Render-Sequenz-Referenz freigegeben hat — processBlock kann nicht
    // mehr laufen, die Sinks dürfen direkt destruieren.
    rtSink.store (nullptr);
    retiredSink.reset();
    sink.reset();
    disableAudioOnce();  // Preset-Load/Shutdown ohne Phase 1 (5.3)
}

//==============================================================================
juce::String LinkAudioSendModule::getModuleId() const          { return staticModuleId; }
juce::String LinkAudioSendModule::getModuleDisplayName() const { return "Link Audio Send"; }
int LinkAudioSendModule::getStateVersion() const               { return 1; }

//==============================================================================
void LinkAudioSendModule::setLinkAudioContext (LinkClock* clock, const juce::String& initialModuleId)
{
    JUCE_ASSERT_MESSAGE_THREAD

    linkClock = clock;
    currentModuleId = initialModuleId;
}

void LinkAudioSendModule::moduleIdRenamed (const juce::String& newModuleId)
{
    JUCE_ASSERT_MESSAGE_THREAD

    currentModuleId = newModuleId;

    if (sink != nullptr)
        sink->setName (newModuleId);  // live zu den Peers (7.2)
}

void LinkAudioSendModule::setClockBus (const ClockBus* bus) noexcept
{
    clockBus = bus;
}

//==============================================================================
void LinkAudioSendModule::prepareToPlay (double, int samplesPerBlock)
{
    // Message Thread, Audio gestoppt bzw. Node noch nicht im Graph —
    // idempotent (der Graph re-prepariert später mit Ist-Werten).
    const auto maxNumSamples = static_cast<std::size_t> (juce::jmax (1, samplesPerBlock))
                             * static_cast<std::size_t> (juce::jmax (1, getTotalNumInputChannels()));

    interleavedBuffer.resize (maxNumSamples);

    auto* clock = linkClock.get();

    if (clock == nullptr)
        return;  // kein Link-Kontext (Tests) — Modul bleibt reines Pass-Through

    if (sink == nullptr && currentModuleId.isNotEmpty())
    {
        // Sink-Größe in SAMPLES, nicht Frames (7.2)
        sink = clock->createSink (currentModuleId, maxNumSamples);
        clock->enableAudio (true);  // Refcount: erstes Modul aktiviert
        audioEnabled = true;
        sendStatus.store (static_cast<int> (SendStatus::announced), std::memory_order_relaxed);
        rtSink.store (sink.get());  // erst publizieren, wenn der Sink komplett ist
    }
    else if (sink != nullptr)
    {
        sink->requestMaxNumSamples (maxNumSamples);  // wächst nur (Link-No-op sonst)
    }
}

void LinkAudioSendModule::releaseResources()
{
    // Sink bleibt announced — das Modul ist weiterhin Teil des Patches.
}

//==============================================================================
void LinkAudioSendModule::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Epoch-Handshake: Inkrement VOR dem rtSink-Load (seq_cst, Header-Doku)
    blocksProcessed.fetch_add (1);

    // Output = Pass-Through des Inputs: der Graph rendert in-place,
    // der Buffer bleibt unangetastet.

    auto* activeSink = rtSink.load();

    if (activeSink == nullptr)
    {
        sendStatus.store (static_cast<int> (SendStatus::offline), std::memory_order_relaxed);
        return;
    }

    const auto numFrames   = buffer.getNumSamples();
    const auto numChannels = juce::jmin (buffer.getNumChannels(), getTotalNumInputChannels());
    const auto numSamples  = static_cast<std::size_t> (juce::jmax (0, numFrames))
                           * static_cast<std::size_t> (juce::jmax (0, numChannels));

    if (clockBus == nullptr || numSamples == 0 || numSamples > interleavedBuffer.size())
    {
        // Ohne Takt-Bus (Tests) oder bei unerwarteter Blockgröße: announced
        // bleiben — kein Crash, kein Commit mit falscher Zeitbasis.
        sendStatus.store (static_cast<int> (SendStatus::announced), std::memory_order_relaxed);
        return;
    }

    convertToInt16Tpdf (buffer.getArrayOfReadPointers(), numChannels, numFrames,
                        interleavedBuffer.data(), ditherState);

    const auto result = activeSink->commitFromClockState (interleavedBuffer.data(),
                                                          numFrames, numChannels,
                                                          clockBus->current);

    sendStatus.store (static_cast<int> (result == LinkClock::Sink::CommitResult::committed
                                            ? SendStatus::streaming
                                            : SendStatus::announced),
                      std::memory_order_relaxed);
}

void LinkAudioSendModule::convertToInt16Tpdf (const float* const* channelData,
                                              int numChannels, int numFrames,
                                              std::int16_t* dest,
                                              std::uint32_t& ditherState) noexcept
{
    for (int frame = 0; frame < numFrames; ++frame)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            // TPDF = Differenz zweier Uniform-Werte aus dem LCG (3.1):
            // Dreieck über ±1 LSB, Mittelwert 0 — kein rand(), kein Heap.
            ditherState = ditherState * 1664525u + 1013904223u;
            const auto r1 = static_cast<float> (ditherState >> 8) * (1.0f / 16777216.0f);
            ditherState = ditherState * 1664525u + 1013904223u;
            const auto r2 = static_cast<float> (ditherState >> 8) * (1.0f / 16777216.0f);

            const auto dithered = channelData[channel][frame] * 32767.0f + (r1 - r2);

            *dest++ = static_cast<std::int16_t> (
                juce::roundToInt (juce::jlimit (-32768.0f, 32767.0f, dithered)));
        }
    }
}

//==============================================================================
void LinkAudioSendModule::releaseSessionResources()
{
    JUCE_ASSERT_MESSAGE_THREAD

    // Phase 1 (5.3): Audio-Thread sofort vom Sink trennen, Refcount
    // freigeben — die Destruktion folgt racefrei über den Epoch-Handshake.
    rtSink.store (nullptr);
    sendStatus.store (static_cast<int> (SendStatus::offline), std::memory_order_relaxed);
    disableAudioOnce();

    if (sink == nullptr)
        return;

    retiredSink = std::move (sink);
    retireEpoch = blocksProcessed.load();
    retireDeadlineMs = juce::Time::getMillisecondCounter() + retireGraceMs;
    triggerAsyncUpdate();
}

void LinkAudioSendModule::handleAsyncUpdate()
{
    if (retiredSink == nullptr)
        return;

    // Self-Re-Dispatch statt Busy-Poll/Timer (Muster 5.2 Schritt 3): warten,
    // bis nach dem rtSink-Store ein neuer Audio-Block begonnen hat. Läuft
    // kein Audio (Zähler steht), destruiert die Deadline.
    if (blocksProcessed.load() == retireEpoch
        && juce::Time::getMillisecondCounter() < retireDeadlineMs)
    {
        triggerAsyncUpdate();
        return;
    }

    retiredSink.reset();  // Kanal verschwindet jetzt bei den Peers
}

void LinkAudioSendModule::disableAudioOnce()
{
    if (! audioEnabled)
        return;

    audioEnabled = false;

    if (auto* clock = linkClock.get())
        clock->enableAudio (false);  // Refcount: letztes Modul deaktiviert
}

//==============================================================================
LinkAudioSendModule::SendStatus LinkAudioSendModule::getSendStatusForUi() const noexcept
{
    return static_cast<SendStatus> (sendStatus.load (std::memory_order_relaxed));
}

juce::String LinkAudioSendModule::getSinkName() const
{
    JUCE_ASSERT_MESSAGE_THREAD
    return sink != nullptr ? sink->getName() : juce::String();
}

bool LinkAudioSendModule::isSinkRetirePending() const noexcept
{
    return retiredSink != nullptr;
}

void LinkAudioSendModule::flushPendingSinkRetirement()
{
    JUCE_ASSERT_MESSAGE_THREAD
    handleUpdateNowIfNeeded();
}

} // namespace conduit
