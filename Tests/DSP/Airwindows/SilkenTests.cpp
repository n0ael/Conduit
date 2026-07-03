/* ========================================
 *  SilkenTests.cpp — DoD-Tests für den Silken-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Silken.h"

using conduit::airwindows::Silken;

TEST_CASE ("Airwindows Silken: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Silken>();
}

TEST_CASE ("Airwindows Silken: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Silken>();
}

TEST_CASE ("Airwindows Silken: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Silken>();
}
