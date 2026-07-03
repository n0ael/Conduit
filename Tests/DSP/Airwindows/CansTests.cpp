/* ========================================
 *  CansTests.cpp — DoD-Tests für den Cans-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Cans.h"

using conduit::airwindows::Cans;

TEST_CASE ("Airwindows Cans: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Cans>();
}

TEST_CASE ("Airwindows Cans: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Cans>();
}

TEST_CASE ("Airwindows Cans: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Cans>();
}
