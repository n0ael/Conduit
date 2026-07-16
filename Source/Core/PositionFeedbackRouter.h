#pragma once

#include <functional>
#include <map>
#include <set>
#include <vector>

#include <juce_core/juce_core.h>

#include "ControllerProfile.h"
#include "MidiInBindings.h"

namespace conduit::midirig
{

//==============================================================================
/** Wert-getriebenes Positions-Feedback fuer Motorfader/Ribbons (MIDI-Rig M8,
    User-Entscheidungen 16.07.2026) -- das Gegenstueck zum Echo-Pfad: der
    Echo feuert nur auf EINGEHENDE Events, ein Motorfader muss aber auch
    fahren, wenn sich der Software-Wert OHNE Hardware-Ereignis aendert
    (Session-Load, Shift-/Channelstrip-Ebenen-Wechsel, UI-Drag, zweiter
    Controller). Deshalb: pro Tick (60-Hz-Hub-Drain) den gebundenen Wert
    jedes `position`-Feedback-Controls diffen und bei Aenderung senden.

    Semantik (User-Entscheidungen):
    - KONTINUIERLICH spiegeln, aber TOUCH-GATED: solange die Touch-Note des
      Controls gehalten ist (AlphaTrack-Fader: Note 0x68), sendet Conduit
      nichts -- sonst kaempft der Motor gegen den Finger. Beim Loslassen
      faehrt der naechste Tick den aktuellen Software-Wert an.
    - BASISWERT, nicht der modulierte Effektivwert (M5c): der Motor zeigt
      den User-Basiswert; Modulation bleibt UI-Anzeige (kein Motor-Tanzen).
      Das setzt der Besitzer im currentBoundValueFor-Seam um.
    - AKTIVE EBENE: der Wert kommt von der aktuell aktiven Bindung der
      Adresse (Shift-/Channelstrip-Ebenen) -- ein Ebenen-Wechsel laesst den
      Motor auf den Wert der neuen Bank springen (Seam via
      MidiInBindings::activeBindingForAddress).

    Profil-getrieben (Muster PickupLedRouter): ein neuer Controller bekommt
    Motor-Feedback NUR ueber CSV-Eintraege (Feedback-meaning `position` +
    Spalte `touch_number`), kein Code. Headless, deterministisch ueber
    tick() testbar. Message Thread. */
class PositionFeedbackRouter
{
public:
    PositionFeedbackRouter() = default;

    //==========================================================================
    // Seams (Besitzer: GridPage)

    /** Position senden -- der Besitzer baut die MidiMessage: pitchBend ->
        MidiMessage::pitchWheel(address.channel, value14) (der PB-Kanal IST
        die Adresse und kommt aus dem CSV, NICHT vom Geraete-Kanal); cc ->
        controllerEvent(Geraete-Kanal, address.number, value14 >> 7). */
    std::function<void (const FeedbackAddress& address, int value14)> send;

    /** Basiswert der aktiven Bindung einer Send-Adresse (Bindungs-Nummer,
        s. grid::pitchBendBindingNumber), < 0 = keine aktive Bindung
        (Motor bleibt stehen). */
    std::function<float (int bindingNumber, bool isNote)> currentBoundValueFor;

    //==========================================================================
    // Konfiguration

    /** Profil-KOPIE der position-Controls uebernehmen (Muster
        PickupLedRouter: Library-Reloads koennen die Quelle invalidieren). */
    void setProfile (const ControllerProfile& profile);
    void clearProfile();

    /** Zustand verwerfen OHNE Sends (Geraete-/Rollen-Wechsel). */
    void reset();

    //==========================================================================
    // Eingaenge [Message Thread]

    /** Noten des Controller-Geraets (Touch-Gate: Fader angefasst = nicht
        senden). Nur Touch-Noten des Profils werden beachtet. */
    void handleControllerNote (int noteNumber, bool isOn);

    /** true, wenn die Note eine Touch-Note des Profils ist (touch_number
        eines Controls ODER Send-Note eines type=touch-Controls). GridPage
        filtert solche Noten aus dem Bindungs-/Learn-Pfad -- sonst wuerde
        der Griff zum Fader die Touch-Note binden (Learn-Falle). */
    [[nodiscard]] bool isTouchNote (int noteNumber) const noexcept
    {
        return touchNotes.count (noteNumber) > 0;
    }

    /** ~60 Hz (Hub-Tick, NACH midiInBindings.tick()): gebundene Werte
        diffen, Aenderungen senden (Dedupe auf dem 14-bit-Wert). */
    void tick();

    // Feedback-meaning (Konvention "Conduit Controller Profile v1", M8)
    static constexpr const char* kMeaningPosition = "position";

private:
    /** Ein position-Feedback-Control des Profils. */
    struct Entry
    {
        int  bindingNumber = -1;    // Send-Adresse als Bindungs-Nummer
        bool isNote        = false;
        int  touchNumber   = -1;    // -1 = kein Touch-Gate
        FeedbackAddress address;    // die position-Feedback-Adresse
    };

    std::vector<Entry> entries;
    std::set<int> touchNotes;    // alle Touch-Noten des Profils (Filter)
    std::set<int> heldTouches;   // aktuell gehaltene Touch-Noten (Gate)

    std::map<std::pair<int, bool>, int> lastSent;   // Bindungs-Nummer -> value14

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PositionFeedbackRouter)
};

} // namespace conduit::midirig
