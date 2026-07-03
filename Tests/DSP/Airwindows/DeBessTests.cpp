/* ========================================
 *  DeBessTests.cpp — DoD-Tests für den DeBess-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/DeBess.h"

using conduit::airwindows::DeBess;

TEST_CASE ("Airwindows DeBess: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<DeBess>();
}

TEST_CASE ("Airwindows DeBess: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<DeBess>();
}

TEST_CASE ("Airwindows DeBess: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<DeBess>();
}
