#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "UI/NoteCircleFadeTracker.h"

namespace grid = conduit::grid;
using Catch::Approx;

namespace
{
    using VoiceReadout = grid::GridVoiceEngine::VoiceReadout;
}

//==============================================================================
TEST_CASE ("NoteCircleFadeTracker: aktive Stimme hat Deckkraft 1", "[ui]")
{
    conduit::NoteCircleFadeTracker tracker (100);

    tracker.update ({ VoiceReadout { 0, 60, 0.5f } }, 16.0);

    REQUIRE (tracker.circles().size() == 1);
    REQUIRE (tracker.circles()[0].voiceIndex == 0);
    REQUIRE (tracker.circles()[0].rawValue == Approx (0.5f));
    REQUIRE (tracker.circles()[0].opacity == Approx (1.0f));
}

TEST_CASE ("NoteCircleFadeTracker: fadet nach fadeMs auf 0 und wird entfernt", "[ui]")
{
    conduit::NoteCircleFadeTracker tracker (100);

    tracker.update ({ VoiceReadout { 0, 60, 0.5f } }, 0.0);
    REQUIRE (tracker.circles().size() == 1);

    // Stimme nicht mehr aktiv -- volle fadeMs vergangen
    tracker.update ({}, 100.0);
    REQUIRE (tracker.circles().empty());
}

TEST_CASE ("NoteCircleFadeTracker: Deckkraft nimmt linear über fadeMs ab", "[ui]")
{
    conduit::NoteCircleFadeTracker tracker (100);

    tracker.update ({ VoiceReadout { 0, 60, 0.5f } }, 0.0);

    // Nicht mehr aktiv, halbe fadeMs vergangen -> halbe Deckkraft, Position
    // bleibt am zuletzt bekannten Rohwert eingefroren
    tracker.update ({}, 50.0);
    REQUIRE (tracker.circles().size() == 1);
    REQUIRE (tracker.circles()[0].opacity == Approx (0.5f));
    REQUIRE (tracker.circles()[0].rawValue == Approx (0.5f));
}

TEST_CASE ("NoteCircleFadeTracker: Wiederauftauchen setzt Deckkraft zurück auf 1", "[ui]")
{
    conduit::NoteCircleFadeTracker tracker (100);

    tracker.update ({ VoiceReadout { 0, 60, 0.5f } }, 0.0);
    tracker.update ({}, 50.0);
    REQUIRE (tracker.circles()[0].opacity == Approx (0.5f));

    // Erneut angeschlagen (dieselbe Stimme wieder aktiv) -> sofort zurück auf 1
    tracker.update ({ VoiceReadout { 0, 62, 0.7f } }, 10.0);
    REQUIRE (tracker.circles().size() == 1);
    REQUIRE (tracker.circles()[0].opacity == Approx (1.0f));
    REQUIRE (tracker.circles()[0].rawValue == Approx (0.7f));
}

TEST_CASE ("NoteCircleFadeTracker: mehrere Stimmen unabhängig verfolgt", "[ui]")
{
    conduit::NoteCircleFadeTracker tracker (100);

    tracker.update ({ VoiceReadout { 0, 60, 0.2f }, VoiceReadout { 1, 64, 0.6f } }, 0.0);
    REQUIRE (tracker.circles().size() == 2);

    // Stimme 0 loslassen, Stimme 1 bleibt aktiv
    tracker.update ({ VoiceReadout { 1, 64, 0.6f } }, 50.0);

    REQUIRE (tracker.circles().size() == 2);   // Stimme 0 fadet noch
    for (const auto& circle : tracker.circles())
    {
        if (circle.voiceIndex == 0)
            REQUIRE (circle.opacity == Approx (0.5f));
        else
            REQUIRE (circle.opacity == Approx (1.0f));
    }
}

TEST_CASE ("NoteCircleFadeTracker: fadeMs 0 fadet sofort auf 0", "[ui]")
{
    conduit::NoteCircleFadeTracker tracker (0);

    tracker.update ({ VoiceReadout { 0, 60, 0.5f } }, 0.0);
    tracker.update ({}, 1.0);

    REQUIRE (tracker.circles().empty());
}
