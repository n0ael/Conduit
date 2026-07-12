#pragma once

#include <array>

#include "MidiControllerEvent.h"

namespace conduit::midirig
{

//==============================================================================
/**
    NRPN-Zustandsautomat PRO EINGANGSPORT (ADR 006 E4, Rule midirig):
    läuft auf dem MIDI-System-Thread VOR dem Queue-Push und setzt
    MSB/LSB-Paare zu fertigen ControllerEvents zusammen — rohe CC-Halbwerte
    erreichen die Queue nie. Pure Zustandsmaschine ohne JUCE-Port-Bezug,
    headless Catch2-testbar. Zustand PRO KANAL (1..16).

    Semantik:
      CC 99/98  — NRPN-Adresse MSB/LSB (aktiviert NRPN)     → consumed
      CC 101/100 — RPN-Adresse (deaktiviert NRPN; 127/127 = Null) → consumed
      CC 6      — Data Entry MSB: bei aktiver NRPN-Adresse ein Event
                  {kind=nrpn, value=msb, is14Bit=false}; ohne aktive
                  Adresse passthrough (Plain-Data-Entry-Geräte)
      CC 38     — Data Entry LSB: bei aktiver Adresse Event
                  {value=(msb<<7)|lsb, is14Bit=true}; sonst passthrough
    Eine 14-bit-Fahrt erzeugt also ZWEI Events (MSB-only, dann voll) —
    der letzte gewinnt beim Konsumenten (Dedupe/Glättung dort).
    CC 96/97 (Increment/Decrement) sind in M2 bewusst out of scope und
    laufen als passthrough durch.
*/
class NrpnAssembler
{
public:
    enum class Result
    {
        passthrough,   // normale CC — unverändert weiterreichen
        consumed,      // Teil einer (N)RPN-Sequenz, kein Event
        event          // fertiges NRPN-Event in `out` geschrieben
    };

    /** [MIDI-System-Thread] channel 1..16, cc/value 0..127. */
    Result feed (int channel, int cc, int value, midi::ControllerEvent& out) noexcept
    {
        if (channel < 1 || channel > 16)
            return Result::passthrough;

        auto& state = channels[static_cast<std::size_t> (channel - 1)];

        switch (cc)
        {
            case 99:  state.nrpnMsb = value; state.nrpnActive = true;  return Result::consumed;
            case 98:  state.nrpnLsb = value; state.nrpnActive = true;  return Result::consumed;
            case 101:                        state.nrpnActive = false; return Result::consumed;
            case 100:                        state.nrpnActive = false; return Result::consumed;

            case 6:
                if (! state.nrpnActive)
                    return Result::passthrough;
                state.dataMsb = value;
                out = makeEvent (channel, state, value, false);
                return Result::event;

            case 38:
                if (! state.nrpnActive)
                    return Result::passthrough;
                out = makeEvent (channel, state, (state.dataMsb << 7) | value, true);
                return Result::event;

            default:
                return Result::passthrough;
        }
    }

private:
    struct ChannelState
    {
        int  nrpnMsb = 0;
        int  nrpnLsb = 0;
        int  dataMsb = 0;
        bool nrpnActive = false;
    };

    static midi::ControllerEvent makeEvent (int channel, const ChannelState& state,
                                            int value, bool is14Bit) noexcept
    {
        midi::ControllerEvent event;
        event.kind    = midi::ControllerEvent::Kind::nrpn;
        event.channel = channel;
        event.number  = state.nrpnMsb * 128 + state.nrpnLsb;
        event.value   = value;
        event.is14Bit = is14Bit;
        return event;
    }

    std::array<ChannelState, 16> channels {};
};

} // namespace conduit::midirig
