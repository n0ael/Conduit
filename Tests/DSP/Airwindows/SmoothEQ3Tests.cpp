/* ========================================
 *  SmoothEQ3Tests.cpp — DoD-Tests für den SmoothEQ3-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/SmoothEQ3.h"

using conduit::airwindows::SmoothEQ3;

TEST_CASE ("Airwindows SmoothEQ3: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<SmoothEQ3>();
}

TEST_CASE ("Airwindows SmoothEQ3: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<SmoothEQ3>();
}

TEST_CASE ("Airwindows SmoothEQ3: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<SmoothEQ3>();
}
