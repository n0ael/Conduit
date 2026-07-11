#include <algorithm>
#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/GridPhysics.h"

namespace grid = conduit::grid;

namespace
{
    // Simuliert n Ticks à dt Sekunden und liefert die Extremwerte der
    // Feder-Position (Überschwing-Nachweis).
    struct Trajectory
    {
        float minPos = 0.0f;
        float maxPos = 0.0f;
        float finalPos = 0.0f;
    };

    Trajectory runSpring (grid::SpringState state, float target,
                          const grid::SpringParams& params, int ticks, float dt)
    {
        Trajectory result { state.pos, state.pos, state.pos };

        for (int i = 0; i < ticks; ++i)
        {
            grid::stepSpring (state, target, params, dt);
            result.minPos = std::min (result.minPos, state.pos);
            result.maxPos = std::max (result.maxPos, state.pos);
        }

        result.finalPos = state.pos;
        return result;
    }
}

//==============================================================================
TEST_CASE ("stepSpring: konvergiert aufs Ziel", "[gridphysics]")
{
    grid::SpringParams params;   // Defaults: force 400, mass 1, inertia 0.4

    const auto run = runSpring ({ 0.0f, 0.0f }, 1.0f, params, 240, 1.0f / 60.0f);
    CHECK (run.finalPos == Catch::Approx (1.0f).margin (1.0e-3));
}

TEST_CASE ("stepSpring: inertia 0 kriecht ohne Überschwingen", "[gridphysics]")
{
    grid::SpringParams params;
    params.inertia01 = 0.0f;

    const auto run = runSpring ({ 0.0f, 0.0f }, 1.0f, params, 240, 1.0f / 60.0f);
    CHECK (run.finalPos == Catch::Approx (1.0f).margin (1.0e-3));
    CHECK (run.maxPos < 1.005f);   // nie nennenswert über das Ziel hinaus
}

TEST_CASE ("stepSpring: hohe inertia schwingt über das Ziel hinaus", "[gridphysics]")
{
    grid::SpringParams params;
    params.inertia01 = 0.9f;

    const auto run = runSpring ({ 0.0f, 0.0f }, 1.0f, params, 480, 1.0f / 60.0f);
    CHECK (run.finalPos == Catch::Approx (1.0f).margin (5.0e-3));
    CHECK (run.maxPos > 1.1f);     // deutliches Überschwingen (Feder-Charakter)
}

TEST_CASE ("stepSpring: forceScale 0 bewegt nichts, dämpft aber aus", "[gridphysics]")
{
    grid::SpringParams params;

    grid::SpringState state { 0.5f, 2.0f };   // Restgeschwindigkeit
    for (int i = 0; i < 240; ++i)
        grid::stepSpring (state, 1.0f, params, 1.0f / 60.0f, 0.0f);

    // Keine Zielkraft: die Dämpfung lässt die Bewegung auslaufen, die
    // Position driftet nicht weiter Richtung Ziel.
    CHECK (std::abs (state.vel) < 1.0e-3f);
}

TEST_CASE ("stepSpring: große dt bleiben stabil (Substeps)", "[gridphysics]")
{
    grid::SpringParams params;
    params.force = 3000.0f;   // steifste Feder der Dev-Panel-Range

    grid::SpringState state { 0.0f, 0.0f };
    for (int i = 0; i < 30; ++i)
        grid::stepSpring (state, 1.0f, params, 0.1f);   // 100-ms-Frames

    CHECK (std::isfinite (state.pos));
    CHECK (state.pos == Catch::Approx (1.0f).margin (1.0e-2));
}

//==============================================================================
TEST_CASE ("GridGravity: Bypass — effectiveX folgt dem Finger exakt", "[gridphysics]")
{
    grid::GridGravity gravity;
    grid::GridGravity::Config config;
    config.delayMs = 100;
    gravity.setConfig (config);

    gravity.onDown (1, 2.3f, 2.0f);
    CHECK (gravity.effectiveX (1, 2.3f) == Catch::Approx (2.3f));

    // Vor Ablauf des Delays: kein Eingriff, Touch-Wert 1:1.
    gravity.tick (0.05f);
    gravity.onMove (1, 2.4f);
    CHECK (gravity.effectiveX (1, 2.4f) == Catch::Approx (2.4f));

    // Unbekannte Finger-Id liefert den Fallback.
    CHECK (gravity.effectiveX (99, 7.7f) == Catch::Approx (7.7f));
}

TEST_CASE ("GridGravity: Stillstand zieht auf den perfekten Pitch", "[gridphysics]")
{
    grid::GridGravity gravity;
    grid::GridGravity::Config config;
    config.delayMs = 100;
    config.fadeMs  = 100;
    config.movementThresholdPadsPerSec = 0.5f;
    gravity.setConfig (config);

    // Finger 0.4 Pads neben dem Anker-Raster (Magnetziel = 2.0).
    gravity.onDown (1, 2.4f, 2.0f);

    for (int i = 0; i < 180; ++i)   // 3 s Stillstand à 60 Hz
        gravity.tick (1.0f / 60.0f);

    CHECK (gravity.effectiveX (1, 2.4f) == Catch::Approx (2.0f).margin (0.02));
    CHECK (gravity.pullAmount (1) == Catch::Approx (1.0f));
}

TEST_CASE ("GridGravity: Magnetziel rastet aufs Anker-Raster, nicht auf 0", "[gridphysics]")
{
    grid::GridGravity gravity;
    grid::GridGravity::Config config;
    config.delayMs = 0;
    config.fadeMs  = 1;
    gravity.setConfig (config);

    // Anker bei 2.5 (Pad-Zentrum), Finger 1.3 Pads daneben → Ziel 2.5 + 1 = 3.5.
    gravity.onDown (7, 3.8f, 2.5f);

    for (int i = 0; i < 240; ++i)
        gravity.tick (1.0f / 60.0f);

    CHECK (gravity.effectiveX (7, 3.8f) == Catch::Approx (3.5f).margin (0.02));
}

TEST_CASE ("GridGravity: Überschwingen um den perfekten Pitch", "[gridphysics]")
{
    grid::GridGravity gravity;
    grid::GridGravity::Config config;
    config.delayMs = 0;
    config.fadeMs  = 1;      // Magnet greift praktisch sofort mit voller Kraft
    config.spring.inertia01 = 0.95f;
    gravity.setConfig (config);

    gravity.onDown (1, 2.4f, 2.0f);   // Ziel 2.0, Annäherung von oben

    auto minSeen = 10.0f;
    for (int i = 0; i < 240; ++i)
    {
        gravity.tick (1.0f / 60.0f);
        minSeen = std::min (minSeen, gravity.effectiveX (1, 2.4f));
    }

    CHECK (minSeen < 1.95f);   // unter das Ziel durchgeschwungen
    CHECK (gravity.effectiveX (1, 2.4f) == Catch::Approx (2.0f).margin (0.03));
}

TEST_CASE ("GridGravity: aktive Bewegung löst den Magneten", "[gridphysics]")
{
    grid::GridGravity gravity;
    grid::GridGravity::Config config;
    config.delayMs = 50;
    config.fadeMs  = 50;
    config.movementThresholdPadsPerSec = 0.5f;
    gravity.setConfig (config);

    gravity.onDown (1, 2.4f, 2.0f);

    // Magnet greifen lassen …
    for (int i = 0; i < 120; ++i)
        gravity.tick (1.0f / 60.0f);
    REQUIRE (gravity.pullAmount (1) == Catch::Approx (1.0f));

    // … dann ein schneller Wisch (deutlich über der Schwelle pro Tick).
    auto x = 2.4f;
    for (int i = 0; i < 30; ++i)
    {
        x += 0.05f;   // 3 Pads/s bei 60 Hz
        gravity.onMove (1, x);
        gravity.tick (1.0f / 60.0f);
    }

    // Kraft ist ausgeblendet, der effektive Wert läuft dem Finger nach.
    CHECK (gravity.pullAmount (1) == Catch::Approx (0.0f));

    for (int i = 0; i < 60; ++i)   // ausschwingen lassen, Finger still
        gravity.tick (1.0f / 60.0f);

    // Nach der Beruhigung greift der Magnet erneut — jetzt auf das Raster
    // um den neuen Touch (x ≈ 3.9 → Ziel 4.0).
    for (int i = 0; i < 240; ++i)
        gravity.tick (1.0f / 60.0f);

    CHECK (gravity.effectiveX (1, x) == Catch::Approx (std::round (x - 2.0f) + 2.0f).margin (0.05));
}

TEST_CASE ("GridGravity: onUp räumt den Finger ab", "[gridphysics]")
{
    grid::GridGravity gravity;
    gravity.onDown (1, 1.0f, 1.0f);
    REQUIRE (gravity.hasFingers());

    gravity.onUp (1);
    CHECK_FALSE (gravity.hasFingers());
    CHECK (gravity.effectiveX (1, 5.5f) == Catch::Approx (5.5f));
}

TEST_CASE ("GridGravity: clear entfernt alle Finger", "[gridphysics]")
{
    grid::GridGravity gravity;
    gravity.onDown (1, 1.0f, 1.0f);
    gravity.onDown (2, 2.0f, 2.0f);

    gravity.clear();
    CHECK_FALSE (gravity.hasFingers());
}
