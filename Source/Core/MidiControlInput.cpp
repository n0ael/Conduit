#include "MidiControlInput.h"

namespace conduit::grid
{

MidiControlInput::MidiControlInput()
{
    startTimerHz (kPumpHz);
}

MidiControlInput::~MidiControlInput()
{
    stopTimer();
    closeDevice();
}

juce::Array<juce::MidiDeviceInfo> MidiControlInput::availableDevices()
{
    return juce::MidiInput::getAvailableDevices();
}

bool MidiControlInput::openDevice (const juce::String& identifier)
{
    closeDevice();

    input = juce::MidiInput::openDevice (identifier, this);
    if (input == nullptr)
        return false;

    input->start();
    return true;
}

void MidiControlInput::closeDevice()
{
    if (input != nullptr)
    {
        input->stop();
        input.reset();
    }
}

juce::String MidiControlInput::currentDeviceIdentifier() const
{
    return input != nullptr ? input->getIdentifier() : juce::String();
}

juce::String MidiControlInput::currentDeviceName() const
{
    return input != nullptr ? input->getName() : juce::String();
}

void MidiControlInput::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    // MIDI-SYSTEM-Thread: nur CCs interessieren, wait-free in die Queue --
    // volle Queue verwirft (aeltere Werte sind bei Controller-Fluten ohnehin
    // obsolet, der Timer entleert 60x pro Sekunde).
    if (! message.isController())
        return;

    queue.push ({ message.getChannel(), message.getControllerNumber(),
                  message.getControllerValue() });
}

void MidiControlInput::timerCallback()
{
    CcEvent event;
    while (queue.pop (event))
        if (onCcReceived != nullptr)
            onCcReceived (event.channel, event.cc, event.value);

    if (onTick != nullptr)
        onTick();
}

} // namespace conduit::grid
