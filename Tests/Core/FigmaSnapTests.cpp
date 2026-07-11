#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/FigmaSnap.h"

namespace grid = conduit::grid;
using Catch::Approx;

//==============================================================================
TEST_CASE ("FigmaSnap: rastet auf die Mittel-Achse eines anderen Elements", "[grid][figmasnap]")
{
    // Anderes Element: Zentrum bei (0.5, 0.5). Bewegtes Element (Breite 0.2)
    // knapp daneben -> Zentrum rastet auf 0.5.
    const juce::Rectangle<float> moving { 0.41f, 0.1f, 0.2f, 0.1f };   // Zentrum x = 0.51
    const std::vector<juce::Rectangle<float>> others { { 0.4f, 0.4f, 0.2f, 0.2f } };

    const auto result = grid::FigmaSnap::snap (moving, others, 0.02f, 0.02f);

    REQUIRE (result.snappedX);
    REQUIRE (result.guideX == Approx (0.5f));
    REQUIRE (result.x == Approx (0.4f));   // Zentrum 0.5 - Breite/2
    REQUIRE_FALSE (result.snappedY);       // y weit weg -> frei
    REQUIRE (result.y == Approx (0.1f));
}

TEST_CASE ("FigmaSnap: ausserhalb der Schwelle bleibt die Position frei", "[grid][figmasnap]")
{
    const juce::Rectangle<float> moving { 0.3f, 0.3f, 0.2f, 0.1f };
    const std::vector<juce::Rectangle<float>> others { { 0.4f, 0.6f, 0.2f, 0.2f } };

    const auto result = grid::FigmaSnap::snap (moving, others, 0.02f, 0.02f);

    REQUIRE_FALSE (result.snappedX);
    REQUIRE_FALSE (result.snappedY);
    REQUIRE (result.x == Approx (0.3f));
    REQUIRE (result.y == Approx (0.3f));
}

TEST_CASE ("FigmaSnap: centreCandidates bietet Mittelpunkt und Verlaengerungen an", "[grid][figmasnap]")
{
    const auto candidates = grid::FigmaSnap::centreCandidates ({ 0.2f, 0.6f });

    // Achs-Fluchten
    REQUIRE (std::find_if (candidates.begin(), candidates.end(),
                 [] (float c) { return std::abs (c - 0.2f) < 1.0e-5f; }) != candidates.end());
    REQUIRE (std::find_if (candidates.begin(), candidates.end(),
                 [] (float c) { return std::abs (c - 0.6f) < 1.0e-5f; }) != candidates.end());

    // Gleicher Abstand: Mittelpunkt (0.4) + Verlaengerungen (1.0 und -0.2)
    REQUIRE (std::find_if (candidates.begin(), candidates.end(),
                 [] (float c) { return std::abs (c - 0.4f) < 1.0e-5f; }) != candidates.end());
    REQUIRE (std::find_if (candidates.begin(), candidates.end(),
                 [] (float c) { return std::abs (c - 1.0f) < 1.0e-5f; }) != candidates.end());
    REQUIRE (std::find_if (candidates.begin(), candidates.end(),
                 [] (float c) { return std::abs (c - (-0.2f)) < 1.0e-5f; }) != candidates.end());
}

TEST_CASE ("FigmaSnap: gleicher Abstand zwischen zwei Nachbarn rastet den Mittelpunkt", "[grid][figmasnap]")
{
    // Zwei Elemente mit Zentren 0.2 und 0.6 -> Mitte 0.4 wird angeboten.
    const juce::Rectangle<float> moving { 0.31f, 0.5f, 0.2f, 0.1f };   // Zentrum x = 0.41
    const std::vector<juce::Rectangle<float>> others {
        { 0.1f, 0.1f, 0.2f, 0.1f },   // Zentrum x = 0.2
        { 0.5f, 0.1f, 0.2f, 0.1f },   // Zentrum x = 0.6
    };

    const auto result = grid::FigmaSnap::snap (moving, others, 0.02f, 0.02f);

    REQUIRE (result.snappedX);
    REQUIRE (result.guideX == Approx (0.4f));
    REQUIRE (result.x + moving.getWidth() * 0.5f == Approx (0.4f));
}

TEST_CASE ("FigmaSnap: der naechste Kandidat gewinnt", "[grid][figmasnap]")
{
    // Kandidaten 0.5 (Flucht), 0.51 (Paar-Mittelpunkt) und 0.52 (zweite
    // Flucht) -- bewegtes Zentrum 0.517 liegt eindeutig am naechsten an 0.52.
    const juce::Rectangle<float> moving { 0.417f, 0.5f, 0.2f, 0.1f };   // Zentrum 0.517
    const std::vector<juce::Rectangle<float>> others {
        { 0.4f, 0.1f, 0.2f, 0.1f },     // Zentrum 0.5
        { 0.42f, 0.3f, 0.2f, 0.1f },    // Zentrum 0.52
    };

    const auto result = grid::FigmaSnap::snap (moving, others, 0.03f, 0.03f);

    REQUIRE (result.snappedX);
    REQUIRE (result.guideX == Approx (0.52f).margin (1.0e-4));
}
