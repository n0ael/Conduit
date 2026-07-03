/* ========================================
 *  TapeHack2Tests.cpp — DoD-Tests für den TapeHack2-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/TapeHack2.h"

using conduit::airwindows::TapeHack2;

TEST_CASE ("Airwindows TapeHack2: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<TapeHack2>();
}

TEST_CASE ("Airwindows TapeHack2: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<TapeHack2>();
}

TEST_CASE ("Airwindows TapeHack2: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<TapeHack2>();
}
