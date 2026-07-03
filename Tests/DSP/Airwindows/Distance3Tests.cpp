/* ========================================
 *  Distance3Tests.cpp — DoD-Tests für den Distance3-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Distance3.h"

using conduit::airwindows::Distance3;

TEST_CASE ("Airwindows Distance3: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Distance3>();
}

TEST_CASE ("Airwindows Distance3: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Distance3>();
}

TEST_CASE ("Airwindows Distance3: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Distance3>();
}
