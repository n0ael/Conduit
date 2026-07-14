#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <map>

#include "Core/MidiInBindings.h"

namespace grid = conduit::grid;
using Catch::Approx;

namespace
{
    // Kleiner Fahrstand: haelt Control-Werte und zeichnet Anwendungen auf.
    struct Rig
    {
        grid::MidiInBindings bindings;
        std::map<int, float> values;   // Key = controlId (ein Layer, Achse 0)
        std::vector<std::pair<int, float>> applied;

        void tick()
        {
            bindings.tick (
                [this] (const grid::MacroControlKey& key) { return values[key.controlId]; },
                [this] (const grid::MacroControlKey& key, float v)
                {
                    values[key.controlId] = v;
                    applied.emplace_back (key.controlId, v);
                });
        }

        void ticks (int n) { for (int i = 0; i < n; ++i) tick(); }
    };

    constexpr grid::MacroControlKey keyFor (int id) { return { grid::MacroControlKey::diy, id, 0 }; }
}

//==============================================================================
TEST_CASE ("SoftTakeover: greift bei Naehe oder Kreuzung, sonst nicht", "[grid][midiin]")
{
    grid::SoftTakeover takeover;

    // Weit weg, keine Historie -> kein Zugriff
    REQUIRE_FALSE (takeover.shouldApply (0.8f, 0.1f));

    // Weiter unterhalb -> immer noch nicht
    REQUIRE_FALSE (takeover.shouldApply (0.8f, 0.5f));

    // Kreuzt den aktuellen Wert (0.5 -> 0.9 ueber 0.8) -> ab jetzt aktiv
    REQUIRE (takeover.shouldApply (0.8f, 0.9f));
    REQUIRE (takeover.shouldApply (0.8f, 0.2f));   // bleibt aktiv

    takeover.disengage();
    REQUIRE_FALSE (takeover.shouldApply (0.8f, 0.2f));   // muss neu aufnehmen
}

TEST_CASE ("SoftTakeover: Naehe-Epsilon reicht zum Aufnehmen", "[grid][midiin]")
{
    grid::SoftTakeover takeover;
    REQUIRE (takeover.shouldApply (0.5f, 0.52f));   // |diff| < 0.03
}

//==============================================================================
TEST_CASE ("MidiInBindings: bind ersetzt Key- und CC-Kollisionen", "[grid][midiin]")
{
    grid::MidiInBindings bindings;

    bindings.bind (keyFor (1), 1, 20);
    bindings.bind (keyFor (2), 1, 21);
    REQUIRE (bindings.count() == 2);

    // Gleicher Key, neuer CC -> ersetzt die alte Bindung von Control 1
    bindings.bind (keyFor (1), 1, 30);
    REQUIRE (bindings.count() == 2);
    REQUIRE (bindings.bindingFor (keyFor (1))->cc == 30);

    // Anderer Key, aber derselbe CC wie Control 2 -> Control 2 verliert
    bindings.bind (keyFor (3), 1, 21);
    REQUIRE (bindings.count() == 2);
    REQUIRE (bindings.bindingFor (keyFor (2)) == nullptr);

    bindings.unbind (keyFor (3));
    REQUIRE (bindings.bindingFor (keyFor (3)) == nullptr);
}

TEST_CASE ("MidiInBindings: eingehender CC wird geglaettet angewendet (nach Pickup)", "[grid][midiin]")
{
    Rig rig;
    rig.bindings.bind (keyFor (1), 1, 20);
    rig.values[1] = 0.5f;

    // Erster Wert nahe am Ist-Wert -> Pickup sofort, Glaettung startet dort.
    rig.bindings.handleIncomingCc (1, 20, 64);   // ~0.504
    rig.tick();
    REQUIRE_FALSE (rig.applied.empty());
    REQUIRE (rig.values[1] == Approx (64.0f / 127.0f).margin (0.01));

    // Sprung auf 127: Glaettung faehrt weich hoch (mehrere Ticks, monoton).
    rig.applied.clear();
    rig.bindings.handleIncomingCc (1, 20, 127);
    rig.tick();
    const auto first = rig.values[1];
    REQUIRE (first < 1.0f);      // nicht sofort am Ziel
    REQUIRE (first > 0.5f);      // aber unterwegs

    rig.ticks (40);
    REQUIRE (rig.values[1] == Approx (1.0f).margin (0.005));

    // Monotonie der Fahrt
    for (size_t i = 1; i < rig.applied.size(); ++i)
        REQUIRE (rig.applied[i].second >= rig.applied[i - 1].second);
}

TEST_CASE ("MidiInBindings: kein Parametersprung ohne Pickup", "[grid][midiin]")
{
    Rig rig;
    rig.bindings.bind (keyFor (1), 1, 20);
    rig.values[1] = 0.9f;

    // Externer Fader steht weit unten -> darf nicht anwenden
    rig.bindings.handleIncomingCc (1, 20, 10);
    rig.ticks (10);
    REQUIRE (rig.applied.empty());
    REQUIRE (rig.values[1] == Approx (0.9f));

    // Externer Fader faehrt hoch und KREUZT 0.9 -> ab da wird angewendet
    rig.bindings.handleIncomingCc (1, 20, 121);   // ~0.953, kreuzt 0.9
    rig.ticks (40);
    REQUIRE_FALSE (rig.applied.empty());
    REQUIRE (rig.values[1] == Approx (121.0f / 127.0f).margin (0.01));
}

TEST_CASE ("MidiInBindings: lokaler Touch loest den Takeover", "[grid][midiin]")
{
    Rig rig;
    rig.bindings.bind (keyFor (1), 1, 20);
    rig.values[1] = 0.5f;

    rig.bindings.handleIncomingCc (1, 20, 64);
    rig.tick();
    REQUIRE_FALSE (rig.applied.empty());

    // User fasst das Control an -> naechster externer Wert weit weg greift nicht
    rig.bindings.notifyLocalTouch (keyFor (1));
    rig.values[1] = 0.1f;
    rig.applied.clear();

    rig.bindings.handleIncomingCc (1, 20, 127);
    rig.ticks (10);
    REQUIRE (rig.applied.empty());
}

TEST_CASE ("MidiInBindings: Feedback-Echo-Schnittstelle feuert beim Anwenden", "[grid][midiin]")
{
    Rig rig;
    rig.bindings.bind (keyFor (1), 1, 20);
    rig.values[1] = 0.5f;

    std::vector<std::tuple<int, int, bool, float>> echoes;
    rig.bindings.onFeedbackEcho = [&] (int channel, int cc, bool isNote, float value01)
    { echoes.emplace_back (channel, cc, isNote, value01); };

    rig.bindings.handleIncomingCc (1, 20, 64);
    rig.tick();

    REQUIRE_FALSE (echoes.empty());
    REQUIRE (std::get<0> (echoes.back()) == 1);
    REQUIRE (std::get<1> (echoes.back()) == 20);
    REQUIRE_FALSE (std::get<2> (echoes.back()));
}

TEST_CASE ("MidiInBindings: MIDI-Learn bindet den naechsten CC und meldet ihn", "[grid][midiin]")
{
    Rig rig;
    rig.values[5] = 0.5f;

    std::vector<std::tuple<int, int>> learned;
    rig.bindings.onLearnCompleted = [&] (const grid::MacroControlKey& key, int channel, int cc, bool)
    {
        REQUIRE (key.controlId == 5);
        learned.emplace_back (channel, cc);
    };

    rig.bindings.armLearn (keyFor (5));
    REQUIRE (rig.bindings.isLearnArmed());

    rig.bindings.handleIncomingCc (2, 74, 64);

    REQUIRE_FALSE (rig.bindings.isLearnArmed());
    REQUIRE (learned.size() == 1);
    REQUIRE (std::get<0> (learned[0]) == 2);
    REQUIRE (std::get<1> (learned[0]) == 74);

    const auto* binding = rig.bindings.bindingFor (keyFor (5));
    REQUIRE (binding != nullptr);
    REQUIRE (binding->channel == 2);
    REQUIRE (binding->cc == 74);

    // Der Lern-CC selbst wird normal verarbeitet -- nahe 0.5 -> Pickup sofort.
    rig.tick();
    REQUIRE (rig.values[5] == Approx (64.0f / 127.0f).margin (0.01));
}

TEST_CASE ("MidiInBindings: cancelLearn entschaerft ohne zu binden", "[grid][midiin]")
{
    Rig rig;
    rig.bindings.armLearn (keyFor (7));
    rig.bindings.cancelLearn();

    rig.bindings.handleIncomingCc (1, 40, 100);
    REQUIRE (rig.bindings.bindingFor (keyFor (7)) == nullptr);
}

//==============================================================================
// M4: Note-Bindungen (Controller-Pads)

TEST_CASE ("MidiInBindings: Note-Bindung -- On setzt Velocity, Off setzt 0", "[grid][midiin][midirig]")
{
    Rig rig;
    rig.bindings.bind (keyFor (3), 1, 36, true);   // Pad auf Note 36
    rig.values[3] = 0.0f;   // Control unten -- Pickup greift sofort bei 0-Naehe? Nein: On=1.0.

    // Erstes Event ist ein Off (0.0): seeded die Glaettung bei 0, nahe am
    // Ist-Wert 0 -> Pickup nimmt sofort auf (engaged bleibt sticky).
    rig.bindings.handleIncomingNote (1, 36, 0, false);
    rig.tick();

    rig.bindings.handleIncomingNote (1, 36, 127, true);
    rig.ticks (30);   // Glaettung auslaufen lassen
    REQUIRE (rig.values[3] == Approx (1.0f).margin (0.01));

    rig.bindings.handleIncomingNote (1, 36, 0, false);
    rig.ticks (30);
    REQUIRE (rig.values[3] == Approx (0.0f).margin (0.01));

    // Halbe Velocity -> halber Zielwert (Momentary + Velocity).
    rig.bindings.handleIncomingNote (1, 36, 64, true);
    rig.ticks (30);
    REQUIRE (rig.values[3] == Approx (64.0f / 127.0f).margin (0.01));
}

TEST_CASE ("MidiInBindings: Note und CC mit gleicher Nummer sind getrennte Adressraeume", "[grid][midiin][midirig]")
{
    Rig rig;
    rig.bindings.bind (keyFor (1), 1, 40, false);   // CC 40
    rig.bindings.bind (keyFor (2), 1, 40, true);    // Note 40 -- darf CC 40 nicht verdraengen

    REQUIRE (rig.bindings.count() == 2);
    REQUIRE (rig.bindings.bindingFor (keyFor (1)) != nullptr);
    REQUIRE (rig.bindings.bindingFor (keyFor (2)) != nullptr);

    // Eingehende Note beruehrt nur die Note-Bindung.
    rig.values[1] = 0.0f;
    rig.values[2] = 0.0f;
    rig.bindings.handleIncomingNote (1, 40, 0, false);   // Off = 0, Pickup sofort
    rig.tick();
    rig.bindings.handleIncomingNote (1, 40, 127, true);
    for (int i = 0; i < 30; ++i)
        rig.tick();

    REQUIRE (rig.values[2] == Approx (1.0f).margin (0.01));
    REQUIRE (rig.values[1] == Approx (0.0f).margin (0.001));   // CC-Bindung unberuehrt
}

TEST_CASE ("MidiInBindings: Learn bindet eine Note (nur On, nicht Off)", "[grid][midiin][midirig]")
{
    Rig rig;
    rig.values[9] = 0.0f;

    bool learnedAsNote = false;
    rig.bindings.onLearnCompleted = [&] (const grid::MacroControlKey& key, int, int number, bool isNote)
    {
        REQUIRE (key.controlId == 9);
        REQUIRE (number == 48);
        learnedAsNote = isNote;
    };

    rig.bindings.armLearn (keyFor (9));

    // Ein streunendes Note-Off darf das Learn NICHT ausloesen.
    rig.bindings.handleIncomingNote (1, 47, 0, false);
    REQUIRE (rig.bindings.isLearnArmed());

    rig.bindings.handleIncomingNote (1, 48, 100, true);
    REQUIRE_FALSE (rig.bindings.isLearnArmed());
    REQUIRE (learnedAsNote);

    const auto* binding = rig.bindings.bindingFor (keyFor (9));
    REQUIRE (binding != nullptr);
    REQUIRE (binding->isNote);
    REQUIRE (binding->cc == 48);
}
