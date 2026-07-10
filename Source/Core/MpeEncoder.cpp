#include "MpeEncoder.h"

#include <cmath>

namespace conduit::grid
{

MpeEncoder::MpeEncoder() noexcept : MpeEncoder (Config{})
{
}

MpeEncoder::MpeEncoder (const Config& cfg) noexcept
    : config (cfg)
{
}

int MpeEncoder::channelForVoice (int voiceIndex) const noexcept
{
    // Poly-/Mono-Aftertouch (Block B4): Nicht-MPE-Synths hoeren auf EINEM
    // Kanal -- alle Stimmen (Note/Bend/Slide/Pressure) auf masterChannel.
    if (mode != ExpressionMode::mpe)
        return masterChannel();

    return config.memberChannelBase + voiceIndex;
}

int MpeEncoder::masterChannel() const noexcept
{
    return config.memberChannelBase - 1;
}

juce::MidiMessage MpeEncoder::noteOn (int voiceIndex, int note, int velocity) const noexcept
{
    return juce::MidiMessage::noteOn (channelForVoice (voiceIndex), note, (juce::uint8) velocity);
}

juce::MidiMessage MpeEncoder::noteOff (int voiceIndex, int note, int releaseVelocity) const noexcept
{
    return juce::MidiMessage::noteOff (channelForVoice (voiceIndex), note, (juce::uint8) releaseVelocity);
}

juce::MidiMessage MpeEncoder::pitchBend (int voiceIndex, float semitones) const noexcept
{
    const int v = 8192 + (int) std::lround (semitones / config.pitchBendRangeSemitones * 8192.0f);
    return juce::MidiMessage::pitchWheel (channelForVoice (voiceIndex), juce::jlimit (0, 16383, v));
}

juce::MidiMessage MpeEncoder::pressure (int voiceIndex, int note, float value01) const noexcept
{
    const int v = juce::jlimit (0, 127, (int) std::lround (value01 * 127.0f));

    // Block B4: Poly-Aftertouch adressiert die NOTE (0xA0) auf dem einen
    // Kanal; mpe/monoAftertouch senden Channel-Pressure (0xD0) -- bei mpe
    // pro Member-Kanal, bei mono auf dem gemeinsamen Kanal.
    if (mode == ExpressionMode::polyAftertouch)
        return juce::MidiMessage::aftertouchChange (channelForVoice (voiceIndex),
                                                    juce::jlimit (0, 127, note), v);

    return juce::MidiMessage::channelPressureChange (channelForVoice (voiceIndex), v);
}

juce::MidiMessage MpeEncoder::slide (int voiceIndex, float value01) const noexcept
{
    const int v = juce::jlimit (0, 127, (int) std::lround (value01 * 127.0f));
    return juce::MidiMessage::controllerEvent (channelForVoice (voiceIndex), 74, v);
}

juce::MidiMessage MpeEncoder::masterVolume (float value01) const noexcept
{
    const int v = juce::jlimit (0, 127, (int) std::lround (value01 * 127.0f));
    return juce::MidiMessage::controllerEvent (masterChannel(), 7, v);
}

} // namespace conduit::grid
