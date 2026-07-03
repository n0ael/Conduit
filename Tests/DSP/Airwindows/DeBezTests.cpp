/* ========================================
 *  DeBezTests.cpp — DoD-Tests für den DeBez-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/DeBez.h"

using conduit::airwindows::DeBez;

TEST_CASE ("Airwindows DeBez: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<DeBez>();
}

TEST_CASE ("Airwindows DeBez: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<DeBez>();
}

TEST_CASE ("Airwindows DeBez: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<DeBez>();
}
