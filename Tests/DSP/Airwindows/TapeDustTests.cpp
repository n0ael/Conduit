/* ========================================
 *  TapeDustTests.cpp — DoD-Tests für den TapeDust-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/TapeDust.h"

using conduit::airwindows::TapeDust;

TEST_CASE ("Airwindows TapeDust: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<TapeDust>();
}

TEST_CASE ("Airwindows TapeDust: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<TapeDust>();
}

TEST_CASE ("Airwindows TapeDust: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<TapeDust>();
}
