#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Core/LaunchQuantization.h"

using Catch::Approx;
using conduit::LaunchQuant;

//==============================================================================
TEST_CASE ("LaunchQuantization: qBeats-Tabelle (Link-Achse, 1 Takt = 4 Beats)",
           "[launchquant]")
{
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::off) == Approx (0.0));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::bars8) == Approx (32.0));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::bars4) == Approx (16.0));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::bars2) == Approx (8.0));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::bar1) == Approx (4.0));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::half) == Approx (2.0));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::quarter) == Approx (1.0));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::eighth) == Approx (0.5));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::sixteenth) == Approx (0.25));
    REQUIRE (conduit::launchQuantBeats (LaunchQuant::thirtySecond) == Approx (0.125));
}

TEST_CASE ("LaunchQuantization: Persist-Key-Roundtrip und Fallback", "[launchquant]")
{
    for (int i = 0; i < conduit::numLaunchQuants; ++i)
    {
        const auto q = static_cast<LaunchQuant> (i);
        REQUIRE (conduit::launchQuantFromKey (conduit::launchQuantKey (q),
                                              LaunchQuant::off)
                 == q);
    }

    // Unbekannte Keys fallen auf den übergebenen Fallback
    REQUIRE (conduit::launchQuantFromKey ("garbage", LaunchQuant::bar1)
             == LaunchQuant::bar1);
    REQUIRE (conduit::launchQuantFromKey ("", LaunchQuant::off) == LaunchQuant::off);
}

TEST_CASE ("LaunchQuantization: Namen sind eindeutig belegt", "[launchquant]")
{
    for (int i = 0; i < conduit::numLaunchQuants; ++i)
        for (int j = i + 1; j < conduit::numLaunchQuants; ++j)
        {
            const std::string_view a = conduit::launchQuantName (static_cast<LaunchQuant> (i));
            const std::string_view b = conduit::launchQuantName (static_cast<LaunchQuant> (j));
            REQUIRE (a != b);
        }
}
