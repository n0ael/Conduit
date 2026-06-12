#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "CaptureSettings.h"
#include "InputMeter.h"
#include "SampleClock.h"

namespace conduit
{

//==============================================================================
/**
    Engine-Service für das Capture-System — Audio-Pendant zu "Capture MIDI".

    Sitzt als Input-Tap VOR dem Graph: processInputTap() sieht den rohen
    Hardware-Input, bevor clockBus/graph/graphFader den Buffer anfassen.
    Graph-Fades und Modul-Outputs gehören nicht in die Aufzeichnung.

    Konfiguration kommt aus den CaptureSettings (App-Zustand, kein Patch-
    Zustand): prepare() dimensioniert den Capture-Ring nach bufferMinutes,
    gedeckelt durch ramLimitGb; die RT-relevanten Settings-Atomics werden
    pro Block im Input-Tap gelesen.

    Als ICaptureBufferHost beantwortet der Service die Resize-Policy der
    Settings (siehe CaptureSettings-Doku): Aktivitäts-Status, Invalidierung
    (Gates zu, Puffer verwerfen, kein Auto-Export) und Reallokation.

    Stand Baustein 2: Ring wird allokiert, aber vom Audio Thread noch nicht
    beschrieben — Reallokation auf dem Message Thread ist deshalb gefahrlos.
    Baustein 3 (PreRoll-Ring im Tap) braucht dann ein Handoff-Protokoll
    (atomic Swap), bevor der Message Thread Puffer austauschen darf.
*/
class CaptureService : public ICaptureBufferHost
{
public:
    explicit CaptureService (CaptureSettings& settingsToUse);

    /** [Message Thread, aus EngineProcessor::prepareToPlay — Audio steht]
        Resettet die SampleClock (Samplerate-Wechsel invalidiert alle
        Positionen), konfiguriert das Metering und allokiert den Ring
        anhand der Settings. */
    void prepare (double sampleRate, int samplesPerBlock, int numInputChannels);

    /** [Audio Thread] ERSTE Operation in processBlock() — allocation-free,
        lock-free. Misst, taktet und (später) puffert den rohen Input. */
    void processInputTap (const juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept;

    //==========================================================================
    // ICaptureBufferHost [Message Thread] — Gegenstück der Resize-Policy
    [[nodiscard]] bool isAnyChannelActive() const override;
    void invalidateAllBuffers() override;
    void reallocateBuffers() override;

    //==========================================================================
    /** Ring-Dimensionierung: bufferMinutes bei sampleRate, gedeckelt durch
        ramLimitGb über alle Kanäle. Pur und allokationsfrei — testbar ohne
        echte Allokation. */
    [[nodiscard]] static int computeRingCapacitySamples (int bufferMinutes, double sampleRate,
                                                         int numChannels, int ramLimitGb) noexcept;

    [[nodiscard]] int getRingCapacitySamples() const noexcept { return ringBuffer.getNumSamples(); }
    [[nodiscard]] int getRingNumChannels() const noexcept     { return ringBuffer.getNumChannels(); }

    [[nodiscard]] const SampleClock& getSampleClock() const noexcept { return sampleClock; }
    [[nodiscard]] const InputMeter& getInputMeter() const noexcept   { return inputMeter; }

private:
    void allocateRing();

    CaptureSettings& settings;

    SampleClock sampleClock;
    InputMeter  inputMeter;

    double preparedSampleRate = 0.0;  // nur Message Thread (prepare/realloc)
    int preparedChannels = 0;

    // PreRoll-/Capture-Ring — Baustein 3 verdrahtet ihn in den Input-Tap
    juce::AudioBuffer<float> ringBuffer;

    // Gate-Status [Audio schreibt (Baustein 4), Message liest via
    // isAnyChannelActive für die Resize-Policy]
    std::atomic<bool> anyChannelActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureService)
};

} // namespace conduit
