/* ========================================
 *  FatEQTests.cpp — DoD-Tests für den FatEQ-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/FatEQ.h"

using conduit::airwindows::FatEQ;

TEST_CASE ("Airwindows FatEQ: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<FatEQ>();
}

TEST_CASE ("Airwindows FatEQ: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<FatEQ>();
}

TEST_CASE ("Airwindows FatEQ: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<FatEQ>();
}
