#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/ExpressionAxis.h"

namespace grid = conduit::grid;
using Catch::Approx;

//==============================================================================
TEST_CASE ("ExpressionAxis: inaktiver Slot ohne Rohwert/Offset ist outMin-geklemmt", "[grid]")
{
    grid::ExpressionAxis axis;

    REQUIRE_FALSE (axis.isActive (0));
    REQUIRE (juce::exactlyEqual (axis.combined (0), 0.0f));
}

TEST_CASE ("ExpressionAxis: setRaw ohne Offset liefert den reinen Rohwert", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.activate (0);
    axis.setRaw (0, 0.5f);

    REQUIRE (axis.isActive (0));
    REQUIRE (juce::exactlyEqual (axis.combined (0), 0.5f));
}

TEST_CASE ("ExpressionAxis: setOffset addiert/subtrahiert auf den Rohwert", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.activate (0);
    axis.setRaw (0, 0.5f);

    axis.setOffset (0.3f);
    REQUIRE (axis.combined (0) == Approx (0.8f));

    axis.setOffset (-0.3f);
    REQUIRE (axis.combined (0) == Approx (0.2f));
}

TEST_CASE ("ExpressionAxis: combined klemmt auf [outMin, outMax]", "[grid]")
{
    grid::ExpressionAxis axis; // Default-Config: 0..1

    axis.activate (0);
    axis.setRaw (0, 0.9f);
    axis.setOffset (0.5f);
    REQUIRE (juce::exactlyEqual (axis.combined (0), 1.0f));

    // Bipolare Config (PitchBend-Vorbereitung) -- combined() klemmt seit dem
    // Kurven-Grenzen-Fix auf die ResponseCurve-Ausgangsgrenzen, nicht mehr
    // auf die Achsen-Kapazität (Config); die Kurve muss denselben Bereich
    // abbilden, damit der Test weiterhin eine Nicht-Default-Klemmung prüft.
    // setRaw bekommt den normalisierten Kurven-Eingang (nicht mehr Halbtöne
    // direkt) -- passend zum unipolaren PitchBend-Modell (S2c-1).
    grid::ExpressionAxis::Config bendConfig { -48.0f, 48.0f, 48.0f };
    grid::ExpressionAxis bendAxis (bendConfig);
    bendAxis.responseCurve().setOutputRange (-48.0f, 48.0f);

    bendAxis.activate (0);
    bendAxis.setRaw (0, 1.0f);
    bendAxis.setOffset (40.0f);
    REQUIRE (juce::exactlyEqual (bendAxis.combined (0), 48.0f));
}

TEST_CASE ("ExpressionAxis: setOffset klemmt auf ±offsetScale", "[grid]")
{
    grid::ExpressionAxis axis; // Default: offsetScale 1.0

    axis.setOffset (5.0f);
    REQUIRE (juce::exactlyEqual (axis.offset(), 1.0f));

    axis.setOffset (-5.0f);
    REQUIRE (juce::exactlyEqual (axis.offset(), -1.0f));
}

TEST_CASE ("ExpressionAxis: deactivate/reset setzen Aktiv-Status und Rohwerte zurück, Offset bleibt", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.activate (0);
    axis.setRaw (0, 0.7f);
    axis.setOffset (0.2f);

    axis.deactivate (0);
    REQUIRE_FALSE (axis.isActive (0));

    axis.activate (0);
    axis.setRaw (0, 0.7f);

    axis.reset();
    REQUIRE_FALSE (axis.isActive (0));
    // Rohwert zurückgesetzt -- kombiniert entspricht jetzt exakt dem Offset
    REQUIRE (juce::exactlyEqual (axis.combined (0), 0.2f));
    // Offset selbst bleibt unverändert
    REQUIRE (juce::exactlyEqual (axis.offset(), 0.2f));
}

TEST_CASE ("ExpressionAxis: Response-Kurve formt VOR dem Offset", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.responseCurve().setOutputRange (0.0f, 0.5f);
    axis.activate (0);
    axis.setRaw (0, 1.0f);

    REQUIRE (axis.combined (0) == Approx (0.5f));

    // Seit dem Kurven-Grenzen-Fix klemmt combined() auf die Kurven-
    // Ausgangsgrenzen (hier 0.5), nicht mehr auf die Achsen-Kapazität --
    // der Offset schiebt den Rohwert zwar weiter, bleibt aber an der
    // Kurven-Grenze hängen (vorher fälschlich 0.8, über die Kurve hinaus).
    axis.setOffset (0.3f);
    REQUIRE (axis.combined (0) == Approx (0.5f));
}

TEST_CASE ("ExpressionAxis: combined klemmt auf den Kurven-Max, auch bei extrapoliertem Rohwert", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.responseCurve().setOutputRange (0.0f, 0.7f);
    axis.activate (0);
    axis.setRaw (0, 2.0f);   // weit über 1, extrapoliert

    REQUIRE (axis.combined (0) == Approx (0.7f));
}

TEST_CASE ("ExpressionAxis: combined klemmt auf den Kurven-Min, auch bei extrapoliertem Rohwert", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.responseCurve().setOutputRange (0.3f, 1.0f);
    axis.activate (0);
    axis.setRaw (0, -1.0f);

    REQUIRE (axis.combined (0) == Approx (0.3f));
}

TEST_CASE ("ExpressionAxis: combined klemmt korrekt sortiert bei invertierter Kurve (Min > Max)", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.responseCurve().setOutputRange (0.9f, 0.2f);
    axis.activate (0);
    axis.setRaw (0, 5.0f);   // extremer Rohwert

    const auto value = axis.combined (0);
    REQUIRE (value >= 0.2f);
    REQUIRE (value <= 0.9f);
}

TEST_CASE ("ExpressionAxis: Offset schiebt bis an die Kurven-Grenze, nicht darüber", "[grid]")
{
    grid::ExpressionAxis axis;

    axis.responseCurve().setOutputRange (0.0f, 0.7f);
    axis.activate (0);
    axis.setRaw (0, 0.5f);
    axis.setOffset (0.5f);

    REQUIRE (axis.combined (0) == Approx (0.7f));
}
