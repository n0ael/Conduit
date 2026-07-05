#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/Looper/LooperClipMath.h"

using Catch::Approx;
namespace lm = conduit::looper;

//==============================================================================
TEST_CASE ("LooperClipMath: Parität mit loopPhaseBeats bei rate=1", "[looper]")
{
    // rate = 1, anchor = loopEndBeat, kein Reverse, kein Fenster-Offset:
    // clipPhaseBeats muss exakt die bestehende loopPhaseBeats-Arithmetik
    // liefern — die LooperBank ersetzt die Engine ohne Phasen-Änderung.
    constexpr double loopEnd = 128.0;
    constexpr double length  = 16.0;   // 4 Takte
    constexpr double spb     = 24000.0;

    for (double beat : { 0.0, 5.25, 127.999, 128.0, 131.5, 200.0, -3.75 })
    {
        const auto expected = lm::loopPhaseBeats (beat, loopEnd, length);
        REQUIRE (lm::clipPhaseBeats (beat, loopEnd, 1.0, length)
                 == Approx (expected).margin (1.0e-12));
        REQUIRE (lm::clipReadPosition (beat, loopEnd, 1.0, length,
                                       false, 0.0, spb)
                 == Approx (expected * spb).margin (1.0e-6));
    }
}

TEST_CASE ("LooperClipMath: Rate-Wechsel ist positions-kontinuierlich", "[looper]")
{
    constexpr double length = 8.0;
    constexpr double anchor = 40.0;

    for (double beat : { 41.3, 55.0, 100.7 })
        for (double r0 : { 0.25, 1.0, 1.5 })
            for (double r1 : { 0.5, 1.0, 4.0 })
            {
                const auto before = lm::clipPhaseBeats (beat, anchor, r0, length);
                const auto newAnchor
                    = lm::reanchorForRateChange (beat, anchor, r0, r1, length);
                const auto after = lm::clipPhaseBeats (beat, newAnchor, r1, length);

                REQUIRE (after == Approx (before).margin (1.0e-9));

                // Danach läuft die Phase mit r1 weiter
                const auto later = lm::clipPhaseBeats (beat + 1.0, newAnchor, r1, length);
                REQUIRE (later == Approx (lm::wrapBeats (before + r1, length))
                                      .margin (1.0e-9));
            }

    // Ungültige Ziel-Rate lässt den Anker unangetastet
    REQUIRE (lm::reanchorForRateChange (50.0, anchor, 1.0, 0.0, length)
             == Approx (anchor));
}

TEST_CASE ("LooperClipMath: Reverse-Toggle spiegelt die Lese-Position exakt",
           "[looper]")
{
    constexpr double length = 16.0;
    constexpr double anchor = 12.0;
    constexpr double spb    = 12000.0;

    for (double beat : { 12.0, 15.5, 30.25, 99.9 })
        for (double rate : { 0.5, 1.0, 2.0 })
        {
            const auto readBefore = lm::clipReadPosition (beat, anchor, rate, length,
                                                          false, 0.0, spb);

            const auto a1 = lm::reanchorForReverseToggle (beat, anchor, rate, length);
            const auto readAfter = lm::clipReadPosition (beat, a1, rate, length,
                                                         true, 0.0, spb);

            // Die LESE-Position steht im Umschaltmoment exakt still
            REQUIRE (readAfter == Approx (readBefore).margin (1.0e-6));

            // Und läuft danach rückwärts (bei kleinen Schritten, weg vom Wrap)
            const auto phase = lm::clipPhaseBeats (beat, a1, rate, length);
            if (phase > 0.5 && phase < length - 0.5)
            {
                const auto step = 0.1;
                const auto next = lm::clipReadPosition (beat + step, a1, rate, length,
                                                        true, 0.0, spb);
                REQUIRE (next == Approx (readBefore - step * rate * spb).margin (1.0e-6));
            }

            // Symmetrie: zweimal togglen stellt die Vorwärts-Position wieder her
            const auto a2 = lm::reanchorForReverseToggle (beat, a1, rate, length);
            const auto readTwice = lm::clipReadPosition (beat, a2, rate, length,
                                                         false, 0.0, spb);
            REQUIRE (readTwice == Approx (readBefore).margin (1.0e-6));
        }
}

TEST_CASE ("LooperClipMath: ×2/÷2 ändern nur L — Phase bleibt beat-starr",
           "[looper]")
{
    // Gleicher Anker, halbe/doppelte Länge: die Phase ist weiterhin
    // wrap((b−a)·rate, L) — kein Re-Anker nötig, ×2 nach ÷2 (firstHalf)
    // stellt exakt den alten Zustand wieder her.
    constexpr double anchor = 20.0;
    constexpr double beat   = 37.3;

    const auto ph8 = lm::clipPhaseBeats (beat, anchor, 1.0, 32.0);
    const auto ph4 = lm::clipPhaseBeats (beat, anchor, 1.0, 16.0);
    REQUIRE (lm::wrapBeats (ph8, 16.0) == Approx (ph4).margin (1.0e-12));
}

TEST_CASE ("LooperClipMath: windowOffsetForHalve — erste vs. aktuelle Hälfte",
           "[looper]")
{
    constexpr double newLength = 8.0;   // ÷2 von 16 Beats

    SECTION ("firstHalf: Fenster bleibt stehen")
    {
        REQUIRE (lm::windowOffsetForHalve (lm::HalveMode::firstHalf, 0.0,
                                           newLength, 3.0)
                 == Approx (0.0));
        REQUIRE (lm::windowOffsetForHalve (lm::HalveMode::firstHalf, 4.0,
                                           newLength, 13.0)
                 == Approx (4.0));
    }

    SECTION ("currentHalf: Fenster springt auf die spielende Hälfte")
    {
        // Phase in der ersten Hälfte → Offset unverändert
        REQUIRE (lm::windowOffsetForHalve (lm::HalveMode::currentHalf, 0.0,
                                           newLength, 3.0)
                 == Approx (0.0));

        // Phase in der zweiten Hälfte → Offset rückt um newLength vor
        REQUIRE (lm::windowOffsetForHalve (lm::HalveMode::currentHalf, 0.0,
                                           newLength, 11.0)
                 == Approx (8.0));

        // Verschachtelt: zweites ÷2 auf bereits versetztem Fenster
        REQUIRE (lm::windowOffsetForHalve (lm::HalveMode::currentHalf, 8.0,
                                           4.0, 6.5)
                 == Approx (8.0 + 4.0));
    }

    SECTION ("currentHalf: Lese-Kontinuität im ÷2-Moment")
    {
        // Vor ÷2: L=16, Phase 11 → Lese-Beat 11. Nach ÷2 (currentHalf):
        // L=8, Phase wrappt auf 3, Fenster-Offset 8 → Lese-Beat 8+3 = 11.
        constexpr double anchor = 0.0;
        constexpr double beat   = 11.0;

        const auto phaseBefore = lm::clipPhaseBeats (beat, anchor, 1.0, 16.0);
        const auto readBefore  = lm::clipReadBeat (phaseBefore, 16.0, false, 0.0);

        const auto offset = lm::windowOffsetForHalve (lm::HalveMode::currentHalf,
                                                      0.0, 8.0, phaseBefore);
        const auto phaseAfter = lm::clipPhaseBeats (beat, anchor, 1.0, 8.0);
        const auto readAfter  = lm::clipReadBeat (phaseAfter, 8.0, false, offset);

        REQUIRE (readAfter == Approx (readBefore).margin (1.0e-12));
    }
}

TEST_CASE ("LooperClipMath: gridCrossingOffset — sample-genauer Grid-Übertritt",
           "[looper]")
{
    // 120 BPM @ 48 kHz: 24000 Samples pro Beat
    constexpr double beatStep = 1.0 / 24000.0;

    SECTION ("qBeats 0 → sofort am Blockanfang")
    {
        REQUIRE (lm::gridCrossingOffset (17.3, beatStep, 128, 0.0) == 0);
    }

    SECTION ("Grenze exakt am Blockanfang gehört diesem Block")
    {
        REQUIRE (lm::gridCrossingOffset (8.0, beatStep, 128, 4.0) == 0);
    }

    SECTION ("Grenze mitten im Block: exakter Sample-Index")
    {
        // Block startet 100 Samples vor Beat 8 (Grenze bei qBeats=4)
        const auto start = 8.0 - 100.0 * beatStep;
        REQUIRE (lm::gridCrossingOffset (start, beatStep, 128, 4.0) == 100);

        // Dieselbe Grenze, kleinere Blöcke: erst -1, dann im Folgeblock
        REQUIRE (lm::gridCrossingOffset (start, beatStep, 64, 4.0) == -1);
        const auto nextBlock = start + 64.0 * beatStep;
        REQUIRE (lm::gridCrossingOffset (nextBlock, beatStep, 64, 4.0) == 36);
    }

    SECTION ("Grenze hinter dem Block → −1")
    {
        REQUIRE (lm::gridCrossingOffset (8.001, beatStep, 32, 4.0) == -1);
    }

    SECTION ("Feine Raster (1/32 = 0.125 Beats) über Blockgrößen 32–256")
    {
        for (int numSamples : { 32, 64, 128, 256 })
        {
            // Grenze bei Beat 5.125, Block startet 10 Samples davor
            const auto start = 5.125 - 10.0 * beatStep;
            const auto expected = 10 < numSamples ? 10 : -1;
            REQUIRE (lm::gridCrossingOffset (start, beatStep, numSamples, 0.125)
                     == expected);
        }
    }

    SECTION ("Negative Beats (vor Session-Start) rasten korrekt")
    {
        // Grenze bei Beat −4 (qBeats=4): Block startet 50 Samples davor
        const auto start = -4.0 - 50.0 * beatStep;
        REQUIRE (lm::gridCrossingOffset (start, beatStep, 128, 4.0) == 50);
    }

    SECTION ("Degenerierte Eingaben")
    {
        REQUIRE (lm::gridCrossingOffset (1.0, beatStep, 0, 4.0) == -1);
        REQUIRE (lm::gridCrossingOffset (1.0, 0.0, 128, 4.0) == -1);
    }
}
