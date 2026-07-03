/* ========================================
 *  WeightTests.cpp — DoD-Tests für den Weight-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Weight.h"

using conduit::airwindows::Weight;

TEST_CASE ("Airwindows Weight: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Weight>();
}

TEST_CASE ("Airwindows Weight: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Weight>();
}

TEST_CASE ("Airwindows Weight: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Weight>();
}
