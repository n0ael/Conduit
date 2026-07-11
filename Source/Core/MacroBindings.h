#pragma once

#include <map>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "Interfaces/IMidiOutputTarget.h"
#include "ResponseCurve.h"

namespace conduit::grid
{

//==============================================================================
/** Ziel eines Macro-Bindings (Block E, Masterplan): gemeinsames Wert-Modell
    -- der Aufrufer liefert 0..1 (bereits durch die Binding-Kurve geformt),
    die Implementierung uebersetzt in ihr Protokoll. Message Thread. */
class MacroTarget
{
public:
    virtual ~MacroTarget() = default;

    virtual void sendValue (float value01) = 0;

    /** Kurzbeschreibung fuer die Ziel-Zeile im Macro-Panel
        (z. B. "CC 74 / Kanal 1" oder "Wavetable: Osc 1 Pos"). */
    [[nodiscard]] virtual juce::String describe() const = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroTarget)

protected:
    MacroTarget() = default;
};

//==============================================================================
/** MIDI-CC-Ziel: Kanal + CC-Nummer auf den Grid-MIDI-Ausgang. Dedupliziert
    auf 7-bit-Aufloesung (127-Stufen-Ziel bekommt nur echte Aenderungen --
    die Glaettung kontinuierlicher Fahrten ist Sache der Zielschicht,
    Masterplan Block E/G). */
class MidiCcTarget final : public MacroTarget
{
public:
    MidiCcTarget (IMidiOutputTarget& outputToUse, int channelToUse, int ccNumberToUse);

    void sendValue (float value01) override;
    [[nodiscard]] juce::String describe() const override;

    [[nodiscard]] int channel() const noexcept { return midiChannel; }
    [[nodiscard]] int ccNumber() const noexcept { return cc; }

private:
    IMidiOutputTarget& output;
    int midiChannel;   // 1..16
    int cc;            // 0..127
    int lastSent = -1; // 7-bit-Dedupe

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiCcTarget)
};

//==============================================================================
/** Adressiert die Macro-Bindings eines Control-WERTS: Layer (System-Controls
    des XY+Fader-Modus vs. DIY-CC-Baukasten -- deren Control-Ids kollidieren,
    beide Modelle zaehlen ab 1), Control-Id und Achse (XY-Pads haben zwei
    unabhaengige Macro-Quellen: 0 = X bzw. der Primaerwert von Fader/Push/
    Toggle, 1 = Y). */
struct MacroControlKey
{
    enum Layer { system = 0, diy = 1 };

    int layer     = system;
    int controlId = 0;
    int axis      = 0;

    auto operator<=> (const MacroControlKey&) const = default;
};

//==============================================================================
/** Ein Ziel-Slot eines Controls: Kurve (2 Punkte + Kruemmung, OHNE Offset --
    Masterplan Block E) vor dem MacroTarget; target == nullptr = Slot ist
    angelegt, aber noch keinem Ziel zugewiesen. lastInput/lastOutput sind die
    Anzeige-Werte der Compact-View (Punkt auf der Linie). */
struct MacroBinding
{
    MacroBinding() = default;   // deleted Copy-Ctor (Makro) unterdrueckt sonst den Default-Ctor

    std::unique_ptr<MacroTarget> target;
    ResponseCurve curve;
    float lastInput01  = 0.0f;
    float lastOutput01 = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroBinding)
};

//==============================================================================
/** Store aller Macro-Bindings, keyed nach MacroControlKey -- bewusst
    GETRENNT vom CcControlModel (das bleibt UI-/MIDI-freies POD; die Ids
    recyceln bei clear(), daher muss der Besitzer clearControl() mitziehen).
    Laufzeit-only, Persistenz kommt gebuendelt in Block K. Message Thread. */
class MacroBindings
{
public:
    static constexpr int kMaxTargetsPerControl = 16;

    MacroBindings() = default;

    /** Neuen (leeren) Ziel-Slot anlegen -- nullptr, wenn kMaxTargetsPerControl
        erreicht ist. */
    MacroBinding* add (const MacroControlKey& key);

    void remove (const MacroControlKey& key, int index);

    [[nodiscard]] int count (const MacroControlKey& key) const noexcept;
    [[nodiscard]] MacroBinding* get (const MacroControlKey& key, int index) noexcept;

    /** Alle Achsen eines Controls entfernen (Control geloescht bzw. Modell
        geleert -- Id-Recycling!). */
    void clearControl (int layer, int controlId);

    /** Wert-Fluss (Block E): value01 → pro Slot Kurve anwenden (geklemmt auf
        [0,1] -- Macro-Ziele kennen keine Achsen-Kapazitaet daruueber) →
        target->sendValue. Slots ohne Ziel aktualisieren nur die Anzeige. */
    void applyValue (const MacroControlKey& key, float value01);

private:
    std::map<MacroControlKey, std::vector<std::unique_ptr<MacroBinding>>> bindings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroBindings)
};

} // namespace conduit::grid
