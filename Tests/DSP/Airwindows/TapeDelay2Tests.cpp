/* ========================================
 *  TapeDelay2Tests.cpp — DoD-Tests für den TapeDelay2-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/TapeDelay2.h"

using conduit::airwindows::TapeDelay2;

TEST_CASE ("Airwindows TapeDelay2: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<TapeDelay2>();
}

TEST_CASE ("Airwindows TapeDelay2: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<TapeDelay2>();
}

TEST_CASE ("Airwindows TapeDelay2: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<TapeDelay2>();
}
