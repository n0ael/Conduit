/* ========================================
 *  PearEQTests.cpp — DoD-Tests für den PearEQ-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/PearEQ.h"

using conduit::airwindows::PearEQ;

TEST_CASE ("Airwindows PearEQ: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<PearEQ>();
}

TEST_CASE ("Airwindows PearEQ: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<PearEQ>();
}

TEST_CASE ("Airwindows PearEQ: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<PearEQ>();
}
