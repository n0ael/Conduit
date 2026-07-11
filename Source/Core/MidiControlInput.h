#pragma once

#include <functional>
#include <memory>

#include <juce_audio_devices/juce_audio_devices.h>

#include "Util/SpscQueue.h"

namespace conduit::grid
{

//==============================================================================
/** MIDI-EINGANG fuer die Control-Steuerung (Block G): oeffnet einen
    existierenden MIDI-In-Port und pumpt Controller-Events auf den Message
    Thread. Der JUCE-MIDI-Callback laeuft auf einem SYSTEM-Thread --
    Uebergabe strikt ueber SpscQueue (CLAUDE.md 3.1, der einzige
    Inter-Thread-Queue-Baustein), ein 60-Hz-Timer auf dem Message Thread
    entleert sie (letzter Wert einer Flut gewinnt naturgemaess ueber die
    nachgelagerte Glaettung in MidiInBindings).

    Gegenstueck zu MidiDeviceTarget (Ausgang): oeffnet nur existierende
    Ports, erzeugt keinen virtuellen Port. */
class MidiControlInput final : private juce::MidiInputCallback,
                               private juce::Timer
{
public:
    MidiControlInput();
    ~MidiControlInput() override;

    [[nodiscard]] static juce::Array<juce::MidiDeviceInfo> availableDevices();

    bool openDevice (const juce::String& identifier);
    void closeDevice();
    [[nodiscard]] bool isOpen() const noexcept { return input != nullptr; }
    [[nodiscard]] juce::String currentDeviceIdentifier() const;

    /** Port-NAME (nicht Identifier) — unter diesem Namen kennt Ableton das
        Gerät als Track-MIDI-Eingang (Track-Selector, Block H). */
    [[nodiscard]] juce::String currentDeviceName() const;

    /** CC eingetroffen [Message Thread, ~60 Hz gepumpt]. */
    std::function<void (int channel, int cc, int value7bit)> onCcReceived;

    /** Pump-Tick [Message Thread] -- feuert nach dem Entleeren der Queue,
        der Besitzer (GridPage) treibt damit die MidiInBindings-Glaettung. */
    std::function<void()> onTick;

private:
    struct CcEvent
    {
        int channel = 0;
        int cc      = 0;
        int value   = 0;
    };

    void handleIncomingMidiMessage (juce::MidiInput* source,
                                    const juce::MidiMessage& message) override;
    void timerCallback() override;

    std::unique_ptr<juce::MidiInput> input;
    SpscQueue<CcEvent> queue { 512 };

    static constexpr int kPumpHz = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiControlInput)
};

} // namespace conduit::grid
