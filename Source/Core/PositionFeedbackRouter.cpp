#include "PositionFeedbackRouter.h"

namespace conduit::midirig
{

namespace
{
    /** Send-Adresse eines Controls als Bindungs-Nummer (MidiInBindings-
        Namensraum): pitchBend traegt seinen Kanal in der Nummer (M8). */
    int bindingNumberFor (const ControllerControl& control) noexcept
    {
        return control.sendKind == AddressKind::pitchBend
                   ? grid::pitchBendBindingNumber (control.sendChannel)
                   : control.sendNumber;
    }
}

void PositionFeedbackRouter::setProfile (const ControllerProfile& profile)
{
    entries.clear();
    touchNotes.clear();

    for (const auto& control : profile.controls)
    {
        if (control.touchNumber >= 0)
            touchNotes.insert (control.touchNumber);

        // Reine Touch-Sensoren (type=touch): Send-Note nur filtern.
        if (control.type.equalsIgnoreCase (kTypeTouch)
            && control.sendKind == AddressKind::note && control.sendNumber >= 0)
            touchNotes.insert (control.sendNumber);

        for (const auto& feedback : control.feedback)
        {
            if (! feedback.meaning.equalsIgnoreCase (kMeaningPosition))
                continue;

            Entry entry;
            entry.bindingNumber = bindingNumberFor (control);
            entry.isNote        = control.sendKind == AddressKind::note;
            entry.touchNumber   = control.touchNumber;
            entry.address       = feedback;
            entries.push_back (entry);
        }
    }
}

void PositionFeedbackRouter::clearProfile()
{
    entries.clear();
    touchNotes.clear();
}

void PositionFeedbackRouter::reset()
{
    heldTouches.clear();
    lastSent.clear();
}

void PositionFeedbackRouter::handleControllerNote (int noteNumber, bool isOn)
{
    if (touchNotes.count (noteNumber) == 0)
        return;

    if (isOn)
        heldTouches.insert (noteNumber);
    else
        heldTouches.erase (noteNumber);
}

void PositionFeedbackRouter::tick()
{
    if (send == nullptr || currentBoundValueFor == nullptr)
        return;

    for (const auto& entry : entries)
    {
        // Touch-Gate: solange der Finger auf dem Control liegt, gewinnt
        // die Hand -- der naechste Tick nach dem Loslassen faehrt nach.
        if (entry.touchNumber >= 0 && heldTouches.count (entry.touchNumber) > 0)
            continue;

        const auto value = currentBoundValueFor (entry.bindingNumber, entry.isNote);
        if (value < 0.0f)
            continue;   // keine aktive Bindung -- Motor bleibt stehen

        const auto value14 = juce::jlimit (0, 16383,
                                           juce::roundToInt (value * 16383.0f));

        const auto key = std::make_pair (entry.bindingNumber, entry.isNote);
        const auto it  = lastSent.find (key);
        if (it != lastSent.end() && it->second == value14)
            continue;

        lastSent[key] = value14;
        send (entry.address, value14);
    }
}

} // namespace conduit::midirig
