#include "MpeMidiSink.h"

namespace conduit::grid
{

namespace
{
    bool isValidVoiceIndex (int voiceIndex) noexcept
    {
        return voiceIndex >= 0 && voiceIndex < VoiceAllocator::kMaxVoices;
    }
}

MpeMidiSink::MpeMidiSink (IMidiOutputTarget& targetToUse, const MpeEncoder::Config& cfg) noexcept
    : target (targetToUse), encoder (cfg)
{
    activeNote.fill (-1);
}

void MpeMidiSink::voiceStart (int voiceIndex, int note, int velocity)
{
    if (! isValidVoiceIndex (voiceIndex))
        return;

    activeNote[(size_t) voiceIndex] = note;
    target.send (encoder.noteOn (voiceIndex, note, velocity));
}

void MpeMidiSink::voiceStop (int voiceIndex, int releaseVelocity)
{
    if (! isValidVoiceIndex (voiceIndex))
        return;

    auto& note = activeNote[(size_t) voiceIndex];

    if (note >= 0)
    {
        target.send (encoder.noteOff (voiceIndex, note, releaseVelocity));
        note = -1;
    }
}

void MpeMidiSink::voicePitchBend (int voiceIndex, float semitones)
{
    if (isValidVoiceIndex (voiceIndex))
        target.send (encoder.pitchBend (voiceIndex, semitones));
}

void MpeMidiSink::voicePressure (int voiceIndex, float value)
{
    if (! isValidVoiceIndex (voiceIndex))
        return;

    // Poly-Aftertouch (Block B4) adressiert die Note -- ohne aktive Note
    // gibt es nichts zu adressieren (mpe/mono ignorieren den Parameter,
    // der Guard schadet dort nicht: Pressure ohne Note ist Alt-Zustand).
    const auto note = activeNote[(size_t) voiceIndex];
    if (note < 0)
        return;

    target.send (encoder.pressure (voiceIndex, note, value));
}

void MpeMidiSink::voiceSlide (int voiceIndex, float value)
{
    if (isValidVoiceIndex (voiceIndex))
        target.send (encoder.slide (voiceIndex, value));
}

void MpeMidiSink::allNotesOff()
{
    for (int i = 0; i < VoiceAllocator::kMaxVoices; ++i)
    {
        auto& note = activeNote[(size_t) i];

        if (note >= 0)
        {
            target.send (encoder.noteOff (i, note, 0));
            note = -1;
        }
    }
}

void MpeMidiSink::setGlobalVolume (float value)
{
    target.send (encoder.masterVolume (value));
}

void MpeMidiSink::setExpressionMode (ExpressionMode newMode)
{
    if (newMode == encoder.expressionMode())
        return;

    // Erst haengende Noten auf den ALTEN Kanaelen beenden -- nach dem
    // Wechsel zeigten Note-Offs sonst auf die neue Kanalzuteilung.
    allNotesOff();
    encoder.setExpressionMode (newMode);
}

} // namespace conduit::grid
