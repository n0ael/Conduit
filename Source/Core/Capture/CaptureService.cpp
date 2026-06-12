#include "CaptureService.h"

#include <limits>

namespace conduit
{

CaptureService::CaptureService (CaptureSettings& settingsToUse)
    : settings (settingsToUse)
{
}

//==============================================================================
void CaptureService::prepare (double sampleRate, int samplesPerBlock, int numInputChannels)
{
    // Blockgröße braucht erst der Ring-Schreibpfad (Baustein 3)
    juce::ignoreUnused (samplesPerBlock);

    preparedSampleRate = sampleRate;
    preparedChannels   = numInputChannels;

    sampleClock.reset();  // Samplerate-Wechsel invalidiert alle Positionen
    inputMeter.prepare (sampleRate, numInputChannels);
    allocateRing();       // anhand der Settings (bufferMinutes, ramLimitGb)
}

int CaptureService::computeRingCapacitySamples (int bufferMinutes, double sampleRate,
                                                int numChannels, int ramLimitGb) noexcept
{
    if (bufferMinutes <= 0 || sampleRate <= 0.0 || numChannels <= 0 || ramLimitGb <= 0)
        return 0;

    const auto wantedSamples = static_cast<juce::int64> (bufferMinutes) * 60
                             * static_cast<juce::int64> (sampleRate);

    const auto ramLimitBytes    = static_cast<juce::int64> (ramLimitGb) * (1024LL * 1024 * 1024);
    const auto samplesWithinRam = ramLimitBytes / (static_cast<juce::int64> (numChannels)
                                                   * static_cast<juce::int64> (sizeof (float)));

    return static_cast<int> (juce::jmin (wantedSamples, samplesWithinRam,
                                         static_cast<juce::int64> (std::numeric_limits<int>::max())));
}

void CaptureService::allocateRing()
{
    const auto channels = juce::jlimit (0, MAX_CAPTURE_CHANNELS, preparedChannels);
    const auto samples  = computeRingCapacitySamples (settings.getBufferMinutes(),
                                                      preparedSampleRate,
                                                      channels,
                                                      settings.getRamLimitGb());

    if (channels <= 0 || samples <= 0)
    {
        ringBuffer.setSize (0, 0);
        return;
    }

    ringBuffer.setSize (channels, samples, false, true, false);
}

//==============================================================================
void CaptureService::processInputTap (const juce::AudioBuffer<float>& buffer,
                                      int numInputChannels) noexcept
{
    // -- Metering: Peak / RMS / Noise-Floor pro Kanal -------------------------
    inputMeter.process (buffer, numInputChannels);

    // Settings-Snapshot pro Block [Message → Audio, atomics] — noch ohne
    // Wirkung: das Gate (Baustein 4) konsumiert Threshold/Hold/AutoCalibrate,
    // der PreRoll-Ring (Baustein 3) das PreRoll-Fenster
    const auto thresholdDb    = settings.getThresholdDb();
    const auto holdMinutes    = settings.getHoldMinutes();
    const auto preRollSeconds = settings.getPreRollSeconds();
    const auto autoCalibrate  = settings.getAutoCalibrate();
    juce::ignoreUnused (thresholdDb, holdMinutes, preRollSeconds, autoCalibrate);

    // -- [Capture-Baustein 3] PreRoll-Ring: rohen Input in den Ringbuffer -----
    //    (ringBuffer ist bereits allokiert; Schreibpfad braucht ein
    //     Handoff-Protokoll gegen reallocateBuffers(), siehe Klassendoku)

    // -- [Capture-Baustein 4] Gate: Signal-über-Noise-Floor-Detektion ---------
    //    (nutzt inputMeter.getNoiseFloor() als adaptive Schwelle,
    //     setzt anyChannelActive für die Resize-Policy)

    // -- [Capture-Baustein 5] Capture-Trigger / Export -------------------------

    // SampleClock zuletzt: erst wenn alle Bausteine die Samples dieses Blocks
    // verarbeitet haben, wird die neue Position publiziert (release) — Leser,
    // die bis now() konsumieren, sehen garantiert vollständige Daten.
    sampleClock.advance (buffer.getNumSamples());
}

//==============================================================================
bool CaptureService::isAnyChannelActive() const
{
    // Baustein 4 (Gate) setzt das Flag; bis dahin ist nie ein Kanal aktiv
    return anyChannelActive.load (std::memory_order_relaxed);
}

void CaptureService::invalidateAllBuffers()
{
    // Resize-Policy: der User hat den Verlust explizit bestätigt —
    // Gates schließen, Puffer verwerfen, bewusst KEIN Auto-Export.
    anyChannelActive.store (false, std::memory_order_relaxed);  // [Baustein 4: echte Gates schließen]
    ringBuffer.clear();                                         // [Baustein 3: Fill-State zurücksetzen]
}

void CaptureService::reallocateBuffers()
{
    // [Message Thread] Erst nach prepare() sinnvoll — vorher fehlen
    // Samplerate und Kanalzahl
    if (preparedSampleRate > 0.0)
        allocateRing();
}

} // namespace conduit
