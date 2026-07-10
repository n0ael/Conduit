#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "Core/PadGridLayout.h"

namespace grid = conduit::grid;

//==============================================================================
TEST_CASE ("PadGridLayout: noteForPad — isomorphes Raster (Default-Config)", "[grid]")
{
    grid::PadGridLayout layout;

    // Unterste Reihe, Spalte 0 -> lowestNote
    REQUIRE (layout.noteForPad (24) == 48); // rows=4 -> rowFromTop 3 = unterste Reihe, cols=8 -> idx 3*8+0
    // Unterste Reihe, Spalte 1 -> +1 Halbton
    REQUIRE (layout.noteForPad (25) == 49);
    // Oberste Reihe, Spalte 0 (padIndex 0) -> lowestNote + 3*5
    REQUIRE (layout.noteForPad (0) == 48 + 3 * 5);
}

TEST_CASE ("PadGridLayout: padIndexAt — Positions-Mapping und Grenzen", "[grid]")
{
    grid::PadGridLayout layout;

    REQUIRE (layout.padIndexAt (0.0f, 0.0f) == 0);                                // oben-links
    REQUIRE (layout.padIndexAt (0.99f, 0.99f) == layout.cols() * layout.rows() - 1); // unten-rechts
    REQUIRE (layout.padIndexAt (1.5f, 0.5f) == -1);                                // außerhalb
}

TEST_CASE ("PadGridLayout: noteAt konsistent zu padIndexAt/noteForPad", "[grid]")
{
    grid::PadGridLayout layout;

    const auto x = 0.3f;
    const auto y = 0.6f;
    const auto pad = layout.padIndexAt (x, y);

    REQUIRE (pad >= 0);
    REQUIRE (layout.noteAt (x, y) == layout.noteForPad (pad));
    REQUIRE (layout.noteAt (1.5f, 0.5f) == -1);
}

TEST_CASE ("PadGridLayout: pitchBendSemitones — Skala, ungeklemmt (M1b-6)", "[grid]")
{
    grid::PadGridLayout layout;

    REQUIRE (juce::exactlyEqual (layout.pitchBendSemitones (0.3f, 0.3f), 0.0f));

    // Eine Pad-Breite (1/cols) nach rechts -> +semitonesPerPadWidth
    const auto padWidth = 1.0f / (float) layout.cols();
    REQUIRE (juce::exactlyEqual (layout.pitchBendSemitones (0.0f, padWidth), 2.0f));

    // Große Bewegung -> NICHT geklemmt, Betrag über pitchBendRangeSemitones
    // (48) hinaus (die pitchBendAxis/der Encoder klemmen erst am Ausgang).
    REQUIRE (layout.pitchBendSemitones (0.0f, 10.0f) > 48.0f);
    REQUIRE (layout.pitchBendSemitones (10.0f, 0.0f) < -48.0f);
}

TEST_CASE ("PadGridLayout: expressionFromDrag — ungeklemmter Ausdruck relativ zum Aufsetzpunkt", "[grid]")
{
    grid::PadGridLayout layout; // Default: yRangeNorm = 0.5

    // Kein Wisch -> neutral
    REQUIRE (juce::exactlyEqual (layout.expressionFromDrag (0.5f, 0.5f), 0.5f));

    // 0.25 nach oben (normY sinkt) -> volle obere Auslenkung
    REQUIRE (juce::exactlyEqual (layout.expressionFromDrag (0.5f, 0.25f), 1.0f));

    // 0.25 nach unten (normY steigt) -> volle untere Auslenkung
    REQUIRE (juce::exactlyEqual (layout.expressionFromDrag (0.5f, 0.75f), 0.0f));

    // Ungeklemmt: 0.5 nach oben/unten -> über 1 hinaus / unter 0
    REQUIRE (juce::exactlyEqual (layout.expressionFromDrag (0.5f, 0.0f), 1.5f));
    REQUIRE (juce::exactlyEqual (layout.expressionFromDrag (0.5f, 1.0f), -0.5f));
}

TEST_CASE ("PadGridLayout: setYRangeNorm — groesserer Wert = kleinere Auslenkung (Block A2)", "[grid]")
{
    grid::PadGridLayout layout; // Default yRangeNorm = 0.5

    const auto before = layout.expressionFromDrag (0.5f, 0.4f);

    layout.setYRangeNorm (1.0f); // doppelt so gross -> halbe Auslenkung
    const auto afterDouble = layout.expressionFromDrag (0.5f, 0.4f);
    REQUIRE (afterDouble - 0.5f == Catch::Approx ((before - 0.5f) * 0.5f));

    layout.setYRangeNorm (0.25f); // halb so gross -> doppelte Auslenkung
    const auto afterHalf = layout.expressionFromDrag (0.5f, 0.4f);
    REQUIRE (afterHalf - 0.5f == Catch::Approx ((before - 0.5f) * 2.0f));
}

TEST_CASE ("PadGridLayout: setYRangeNorm klemmt auf > 0", "[grid]")
{
    grid::PadGridLayout layout;
    layout.setYRangeNorm (-5.0f);

    // Darf nicht durch 0/negativ teilen -- Ergebnis muss endlich sein.
    REQUIRE (std::isfinite (layout.expressionFromDrag (0.5f, 0.4f)));
}

TEST_CASE ("PadGridLayout: setSemitonesPerPadWidth skaliert pitchBendSemitones linear (Block A3)", "[grid]")
{
    grid::PadGridLayout layout;
    const auto padWidth = 1.0f / (float) layout.cols();

    layout.setSemitonesPerPadWidth (0.5f); // x0.25 ggue. Default (2.0)
    REQUIRE (juce::exactlyEqual (layout.pitchBendSemitones (0.0f, padWidth), 0.5f));

    layout.setSemitonesPerPadWidth (4.0f); // x2 ggue. Default
    REQUIRE (juce::exactlyEqual (layout.pitchBendSemitones (0.0f, padWidth), 4.0f));

    layout.setSemitonesPerPadWidth (16.0f); // x8 ggue. Default
    REQUIRE (juce::exactlyEqual (layout.pitchBendSemitones (0.0f, padWidth), 16.0f));
}
