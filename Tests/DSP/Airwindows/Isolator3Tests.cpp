/* ========================================
 *  Isolator3Tests.cpp — DoD-Tests für den Isolator3-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Isolator3.h"

using conduit::airwindows::Isolator3;

TEST_CASE ("Airwindows Isolator3: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Isolator3>();
}

TEST_CASE ("Airwindows Isolator3: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Isolator3>();
}

TEST_CASE ("Airwindows Isolator3: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Isolator3>();
}
