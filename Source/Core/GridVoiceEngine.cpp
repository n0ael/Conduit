#include "GridVoiceEngine.h"

namespace conduit::grid
{

GridVoiceEngine::GridVoiceEngine (IVoiceSink& sinkToUse, int maxVoices) noexcept
    : sink (sinkToUse), allocator (maxVoices)
{
}

void GridVoiceEngine::noteOn (uint32_t fingerId, int note, int velocity) noexcept
{
    uint32_t stolen = 0;
    const int voiceIndex = allocator.allocate (fingerId, stolen);

    if (voiceIndex < 0)
        return;

    if (stolen != 0)
        sink.voiceStop (voiceIndex, 0);

    voiceActive[(size_t) voiceIndex]  = true;
    rawPressure[(size_t) voiceIndex]  = 0.0f;

    sink.voiceStart (voiceIndex, note, velocity);

    // Ein aktiver Offset gilt sofort auch für frische Noten, nicht erst ab
    // der nächsten Y-Bewegung des Fingers.
    if (! juce::exactlyEqual (pressureOffset, 0.0f))
        sink.voicePressure (voiceIndex, juce::jlimit (0.0f, 1.0f, pressureOffset));
}

void GridVoiceEngine::noteOff (uint32_t fingerId, int releaseVelocity) noexcept
{
    const int voiceIndex = allocator.release (fingerId);

    if (voiceIndex < 0)
        return;

    sink.voiceStop (voiceIndex, releaseVelocity);
    voiceActive[(size_t) voiceIndex] = false;
    rawPressure[(size_t) voiceIndex] = 0.0f;
}

void GridVoiceEngine::setPitchBend (uint32_t fingerId, float semitones) noexcept
{
    const int voiceIndex = allocator.voiceForFinger (fingerId);

    if (voiceIndex >= 0)
        sink.voicePitchBend (voiceIndex, semitones);
}

void GridVoiceEngine::setPressure (uint32_t fingerId, float value01) noexcept
{
    const int voiceIndex = allocator.voiceForFinger (fingerId);

    if (voiceIndex < 0)
        return;

    rawPressure[(size_t) voiceIndex] = value01;
    sink.voicePressure (voiceIndex, juce::jlimit (0.0f, 1.0f, value01 + pressureOffset));
}

void GridVoiceEngine::setSlide (uint32_t fingerId, float value01) noexcept
{
    const int voiceIndex = allocator.voiceForFinger (fingerId);

    if (voiceIndex >= 0)
        sink.voiceSlide (voiceIndex, value01);
}

void GridVoiceEngine::allNotesOff() noexcept
{
    sink.allNotesOff();
    allocator.reset();

    voiceActive.fill (false);
    rawPressure.fill (0.0f);
    // pressureOffset bleibt -- die Ribbon-Stellung hält über Release-All hinweg.
}

void GridVoiceEngine::setGlobalVolume (float value01) noexcept
{
    sink.setGlobalVolume (value01);
}

void GridVoiceEngine::setPressureOffset (float bipolarOffset) noexcept
{
    pressureOffset = juce::jlimit (-1.0f, 1.0f, bipolarOffset);

    for (int i = 0; i < allocator.maxVoices(); ++i)
    {
        if (voiceActive[(size_t) i])
            sink.voicePressure (i, juce::jlimit (0.0f, 1.0f, rawPressure[(size_t) i] + pressureOffset));
    }
}

} // namespace conduit::grid
