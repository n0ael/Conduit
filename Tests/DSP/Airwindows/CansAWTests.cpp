/* ========================================
 *  CansAWTests.cpp — DoD-Tests für den CansAW-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/CansAW.h"

using conduit::airwindows::CansAW;

TEST_CASE ("Airwindows CansAW: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<CansAW>();
}

TEST_CASE ("Airwindows CansAW: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<CansAW>();
}

TEST_CASE ("Airwindows CansAW: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<CansAW>();
}
