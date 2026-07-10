#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/GridSensitivity.h"

namespace grid = conduit::grid;

//==============================================================================
TEST_CASE ("GridSensitivity: 50 ist exakt Faktor 1.0 (heutiges Verhalten)", "[gridsensitivity]")
{
    CHECK (grid::sensitivityToRangeScale (50.0) == Catch::Approx (1.0f));
}

TEST_CASE ("GridSensitivity: 0 und 100 sind die dokumentierten Randwerte", "[gridsensitivity]")
{
    CHECK (grid::sensitivityToRangeScale (0.0) == Catch::Approx (0.25f));
    CHECK (grid::sensitivityToRangeScale (100.0) == Catch::Approx (4.0f));
}

TEST_CASE ("GridSensitivity: streng monoton steigend", "[gridsensitivity]")
{
    const double points[] = { 0.0, 10.0, 25.0, 40.0, 50.0, 60.0, 75.0, 90.0, 100.0 };

    for (std::size_t i = 1; i < std::size (points); ++i)
        CHECK (grid::sensitivityToRangeScale (points[i]) > grid::sensitivityToRangeScale (points[i - 1]));
}

TEST_CASE ("GridSensitivity: nie <= 0", "[gridsensitivity]")
{
    CHECK (grid::sensitivityToRangeScale (-1000.0) > 0.0f);
    CHECK (grid::sensitivityToRangeScale (1000.0) > 0.0f);
}
