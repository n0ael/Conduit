#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace conduit::grid
{

//==============================================================================
/** Ausdrucks-Routing der Y-Achse fuer Nicht-MPE-Synths (Block B4):
    mpe            = heutiges Verhalten (Kanalspreizung, Channel-Pressure pro
                     Member-Kanal),
    polyAftertouch = EIN Kanal (masterChannel), Pressure als Poly-Aftertouch
                     (0xA0) pro Note,
    monoAftertouch = EIN Kanal, Pressure als Channel-Pressure (0xD0) fuer
                     alle Stimmen gemeinsam (letzter Schreiber gewinnt). */
enum class ExpressionMode { mpe, polyAftertouch, monoAftertouch };

//==============================================================================
/**
    Voice-Slot-indizierte Events → MPE-MIDI 1.0 (Lower Zone: Master = Kanal 1,
    Member = Kanäle memberChannelBase .. memberChannelBase+maxVoices-1).

    Zustandslos bis auf den ExpressionMode (Block B4), allocation-free, kein
    I/O — der Sink hält die aktive Note pro Voice, nicht dieser Encoder.
    voiceIndex i → Kanal base + i (mpe) bzw. masterChannel (Poly-/Mono-AT:
    alle Stimmen auf EINEM Kanal — auch PitchBend/Slide, letzter Schreiber
    gewinnt, wie bei jedem Nicht-MPE-Synth).
*/
class MpeEncoder
{
public:
    struct Config
    {
        int   memberChannelBase       = 2;
        int   maxVoices               = 15;
        float pitchBendRangeSemitones = 48.0f; // MPE-Default pro Member-Kanal
    };

    // Clang lehnt eine verschachtelte Config mit In-Class-Defaults als
    // Default-Argument ab ("needed within definition of enclosing class
    // ... outside of member functions") — Default-Ctor delegiert stattdessen
    // in der .cpp, wo MpeEncoder schon vollständig ist.
    MpeEncoder() noexcept;
    explicit MpeEncoder (const Config& cfg) noexcept;

    /** Block B4: Routing-Modus zur Laufzeit umschaltbar. Der Aufrufer
        (MpeMidiSink) beendet vorher haengende Noten -- ein Wechsel bei
        klingenden Stimmen liesse Note-Ons auf den alten Kanaelen zurueck. */
    void setExpressionMode (ExpressionMode newMode) noexcept { mode = newMode; }
    [[nodiscard]] ExpressionMode expressionMode() const noexcept { return mode; }

    int channelForVoice (int voiceIndex) const noexcept; // 1-basiert; mpe: base+i, sonst masterChannel
    int masterChannel() const noexcept;                  // = memberChannelBase - 1 (Lower Zone: 1)

    juce::MidiMessage noteOn    (int voiceIndex, int note, int velocity)        const noexcept;
    juce::MidiMessage noteOff   (int voiceIndex, int note, int releaseVelocity) const noexcept;
    juce::MidiMessage pitchBend (int voiceIndex, float semitones)               const noexcept;
    /** Pressure braucht seit Block B4 die aktive NOTE (Poly-Aftertouch ist
        pro Note adressiert) -- der Sink haelt sie (activeNote). Im
        mpe-/monoAftertouch-Modus wird note ignoriert. */
    juce::MidiMessage pressure  (int voiceIndex, int note, float value01)       const noexcept;
    juce::MidiMessage slide     (int voiceIndex, float value01)                 const noexcept;

    juce::MidiMessage masterVolume (float value01) const noexcept; // CC7 auf masterChannel

private:
    Config config;
    ExpressionMode mode = ExpressionMode::mpe;
};

} // namespace conduit::grid
