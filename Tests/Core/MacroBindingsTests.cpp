#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/MacroBindings.h"
#include "TouchLive/AbletonParamTarget.h"
#include "FakeMidiTarget.h"

namespace grid = conduit::grid;
using Catch::Approx;

namespace
{
    // Test-Double: zeichnet gesendete Werte auf (target-agnostisch).
    struct RecordingTarget final : public grid::MacroTarget
    {
        void sendValue (float value01) override { values.push_back (value01); }
        [[nodiscard]] juce::String describe() const override { return "Recorder"; }

        std::vector<float> values;
    };
}

//==============================================================================
TEST_CASE ("MacroBindings: add/count/get/remove pro Key, Limit 16", "[grid][macro]")
{
    grid::MacroBindings bindings;
    const grid::MacroControlKey key { grid::MacroControlKey::diy, 3, 0 };

    REQUIRE (bindings.count (key) == 0);
    REQUIRE (bindings.get (key, 0) == nullptr);

    for (int i = 0; i < grid::MacroBindings::kMaxTargetsPerControl; ++i)
        REQUIRE (bindings.add (key) != nullptr);

    REQUIRE (bindings.count (key) == grid::MacroBindings::kMaxTargetsPerControl);
    REQUIRE (bindings.add (key) == nullptr);   // 17. Slot abgelehnt

    bindings.remove (key, 0);
    REQUIRE (bindings.count (key) == grid::MacroBindings::kMaxTargetsPerControl - 1);
    REQUIRE (bindings.add (key) != nullptr);   // wieder Platz
}

TEST_CASE ("MacroBindings: Keys trennen Layer, Control und Achse", "[grid][macro]")
{
    grid::MacroBindings bindings;

    bindings.add ({ grid::MacroControlKey::system, 1, 0 });
    bindings.add ({ grid::MacroControlKey::diy, 1, 0 });
    bindings.add ({ grid::MacroControlKey::diy, 1, 1 });

    REQUIRE (bindings.count ({ grid::MacroControlKey::system, 1, 0 }) == 1);
    REQUIRE (bindings.count ({ grid::MacroControlKey::diy, 1, 0 }) == 1);
    REQUIRE (bindings.count ({ grid::MacroControlKey::diy, 1, 1 }) == 1);
    REQUIRE (bindings.count ({ grid::MacroControlKey::system, 1, 1 }) == 0);
}

TEST_CASE ("MacroBindings: clearControl entfernt alle Achsen eines Controls", "[grid][macro]")
{
    grid::MacroBindings bindings;
    bindings.add ({ grid::MacroControlKey::diy, 5, 0 });
    bindings.add ({ grid::MacroControlKey::diy, 5, 1 });
    bindings.add ({ grid::MacroControlKey::diy, 6, 0 });

    bindings.clearControl (grid::MacroControlKey::diy, 5);

    REQUIRE (bindings.count ({ grid::MacroControlKey::diy, 5, 0 }) == 0);
    REQUIRE (bindings.count ({ grid::MacroControlKey::diy, 5, 1 }) == 0);
    REQUIRE (bindings.count ({ grid::MacroControlKey::diy, 6, 0 }) == 1);
}

TEST_CASE ("MacroBindings: applyValue formt durch die Kurve und klemmt auf [0,1]", "[grid][macro]")
{
    grid::MacroBindings bindings;
    const grid::MacroControlKey key { grid::MacroControlKey::system, 2, 0 };

    auto* binding = bindings.add (key);
    REQUIRE (binding != nullptr);

    auto recorder = std::make_unique<RecordingTarget>();
    auto* recorderPtr = recorder.get();
    binding->target = std::move (recorder);

    // Identitaets-Kurve: Wert geht 1:1 durch.
    bindings.applyValue (key, 0.25f);
    REQUIRE (recorderPtr->values.size() == 1);
    REQUIRE (recorderPtr->values.back() == Approx (0.25f));
    REQUIRE (binding->lastInput01 == Approx (0.25f));
    REQUIRE (binding->lastOutput01 == Approx (0.25f));

    // Invertierte Kurve (Range 1..0): Ausgang gespiegelt.
    binding->curve.setOutputRange (1.0f, 0.0f);
    bindings.applyValue (key, 0.25f);
    REQUIRE (recorderPtr->values.back() == Approx (0.75f));

    // Ausgang geklemmt: Range ueber [0,1] hinaus wird begrenzt.
    binding->curve.setOutputRange (0.0f, 2.0f);
    bindings.applyValue (key, 1.0f);
    REQUIRE (recorderPtr->values.back() == Approx (1.0f));
}

TEST_CASE ("MacroBindings: Slots ohne Ziel aktualisieren nur die Anzeige", "[grid][macro]")
{
    grid::MacroBindings bindings;
    const grid::MacroControlKey key { grid::MacroControlKey::diy, 9, 0 };

    auto* binding = bindings.add (key);
    bindings.applyValue (key, 0.5f);   // darf nicht crashen (target == nullptr)

    REQUIRE (binding->lastInput01 == Approx (0.5f));
    REQUIRE (binding->lastOutput01 == Approx (0.5f));
}

//==============================================================================
TEST_CASE ("MidiCcTarget: sendet controllerEvent, dedupliziert auf 7 bit", "[grid][macro]")
{
    grid::FakeMidiTarget fake;
    grid::MidiCcTarget target (fake, 1, 74);

    target.sendValue (0.5f);
    REQUIRE (fake.messages.size() == 1);
    REQUIRE (fake.messages[0].isController());
    REQUIRE (fake.messages[0].getChannel() == 1);
    REQUIRE (fake.messages[0].getControllerNumber() == 74);
    const auto first = fake.messages[0].getControllerValue();
    REQUIRE ((first == 63 || first == 64));

    // Gleicher 7-bit-Wert: kein zweiter Send (Dedupe).
    target.sendValue (0.5f);
    REQUIRE (fake.messages.size() == 1);

    target.sendValue (1.0f);
    REQUIRE (fake.messages.size() == 2);
    REQUIRE (fake.messages[1].getControllerValue() == 127);
}

TEST_CASE ("MidiCcTarget: klemmt Kanal und CC-Nummer", "[grid][macro]")
{
    grid::FakeMidiTarget fake;
    grid::MidiCcTarget target (fake, 99, 200);

    REQUIRE (target.channel() == 16);
    REQUIRE (target.ccNumber() == 127);
}

//==============================================================================
TEST_CASE ("AbletonParamTarget: mapToNative skaliert auf den Live-Bereich", "[grid][macro]")
{
    using T = grid::AbletonParamTarget;

    REQUIRE (T::mapToNative (0.0f, -60.0f, 6.0f, false) == Approx (-60.0f));
    REQUIRE (T::mapToNative (1.0f, -60.0f, 6.0f, false) == Approx (6.0f));
    REQUIRE (T::mapToNative (0.5f, 0.0f, 1.0f, false) == Approx (0.5f));

    // Geklemmt ausserhalb [0,1]
    REQUIRE (T::mapToNative (2.0f, 0.0f, 10.0f, false) == Approx (10.0f));
    REQUIRE (T::mapToNative (-1.0f, 0.0f, 10.0f, false) == Approx (0.0f));

    // Quantisiert: ganze Schritte
    REQUIRE (T::mapToNative (0.5f, 0.0f, 5.0f, true) == Approx (3.0f));   // 2.5 -> round
    REQUIRE (T::mapToNative (0.2f, 0.0f, 5.0f, true) == Approx (1.0f));
}
