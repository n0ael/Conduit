/* ========================================
 *  StonefireTests.cpp — DoD-Tests für den Stonefire-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Stonefire.h"

using conduit::airwindows::Stonefire;

TEST_CASE ("Airwindows Stonefire: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Stonefire>();
}

TEST_CASE ("Airwindows Stonefire: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Stonefire>();
}

TEST_CASE ("Airwindows Stonefire: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Stonefire>();
}
