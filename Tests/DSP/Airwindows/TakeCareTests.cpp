/* ========================================
 *  TakeCareTests.cpp — DoD-Tests für den TakeCare-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/TakeCare.h"

using conduit::airwindows::TakeCare;

TEST_CASE ("Airwindows TakeCare: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<TakeCare>();
}

TEST_CASE ("Airwindows TakeCare: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<TakeCare>();
}

TEST_CASE ("Airwindows TakeCare: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<TakeCare>();
}
