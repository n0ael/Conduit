#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/RingTouchModel.h"

namespace grid = conduit::grid;
using Catch::Approx;

//==============================================================================
TEST_CASE ("RingTouchModel: primäre und Ring-Zuordnung nach Abstand", "[grid]")
{
    grid::RingTouchModel model;

    const auto d1 = model.onDown (1, { 0.0f, 0.0f });
    REQUIRE (d1.kind == grid::RingTouchModel::TouchKind::Primary);

    // Abstand 50 < grabRadiusPx (90) -> wird Ring von Finger 1
    const auto d2 = model.onDown (2, { 50.0f, 0.0f });
    REQUIRE (d2.kind == grid::RingTouchModel::TouchKind::Ring);
    REQUIRE (d2.ringOwner == 1);

    // Weit weg -> eigener primärer Finger
    const auto d3 = model.onDown (3, { 500.0f, 0.0f });
    REQUIRE (d3.kind == grid::RingTouchModel::TouchKind::Primary);
}

TEST_CASE ("RingTouchModel: zweiter Touch nahe an einem primären mit bestehendem Ring wird selbst primär", "[grid]")
{
    grid::RingTouchModel model;
    model.onDown (1, { 0.0f, 0.0f });
    model.onDown (2, { 50.0f, 0.0f }); // Ring von 1

    const auto d3 = model.onDown (3, { 60.0f, 0.0f }); // nah an 1, aber 1 hat schon einen Ring
    REQUIRE (d3.kind == grid::RingTouchModel::TouchKind::Primary);
}

TEST_CASE ("RingTouchModel: Radius moduliert Slide zwischen min-/maxRadiusPx", "[grid]")
{
    grid::RingTouchModel model;
    model.onDown (1, { 0.0f, 0.0f });
    model.onDown (2, { 50.0f, 0.0f });

    const auto atMax = model.onMove (2, { 220.0f, 0.0f }); // maxRadiusPx-Abstand von {0,0}
    REQUIRE (atMax.hasSlide);
    REQUIRE (atMax.owner == 1);
    REQUIRE (atMax.slide01 == Approx (1.0f));

    const auto atMin = model.onMove (2, { 40.0f, 0.0f }); // minRadiusPx-Abstand
    REQUIRE (atMin.hasSlide);
    REQUIRE (atMin.slide01 == Approx (0.0f));

    // Bewegung des primären Fingers selbst interessiert das Model nicht
    const auto primaryMove = model.onMove (1, { 5.0f, 0.0f });
    REQUIRE_FALSE (primaryMove.hasSlide);
}

TEST_CASE ("RingTouchModel: onUp deregistriert Ring bzw. entfernt den primären Finger", "[grid]")
{
    grid::RingTouchModel model;
    model.onDown (1, { 0.0f, 0.0f });
    model.onDown (2, { 50.0f, 0.0f });

    const auto upRing = model.onUp (2);
    REQUIRE (upRing.wasRing);
    REQUIRE_FALSE (upRing.wasPrimary);
    REQUIRE (upRing.ringOwner == 1);

    // Ring ist jetzt inaktiv -- eine weitere Bewegung liefert kein Slide mehr
    const auto moveAfterUp = model.onMove (2, { 100.0f, 0.0f });
    REQUIRE_FALSE (moveAfterUp.hasSlide);

    const auto upPrimary = model.onUp (1);
    REQUIRE (upPrimary.wasPrimary);
    REQUIRE_FALSE (upPrimary.wasRing);
    REQUIRE (upPrimary.primaryFinger == 1);
}

TEST_CASE ("RingTouchModel: activeCircles spiegelt Radius wider", "[grid]")
{
    grid::RingTouchModel model;
    model.onDown (1, { 0.0f, 0.0f });

    auto circles = model.activeCircles();
    REQUIRE (circles.size() == 1);
    REQUIRE (circles[0].radiusPx == Approx (40.0f)); // minRadiusPx, kein Ring aktiv

    model.onDown (2, { 50.0f, 0.0f });
    model.onMove (2, { 100.0f, 0.0f });

    circles = model.activeCircles();
    REQUIRE (circles.size() == 1); // Ring-Finger bekommt keinen eigenen Kreis
    REQUIRE (circles[0].radiusPx == Approx (100.0f));
}
