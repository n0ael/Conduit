/* ========================================
 *  SweetenTests.cpp — DoD-Tests für den Sweeten-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Sweeten.h"

using conduit::airwindows::Sweeten;

TEST_CASE ("Airwindows Sweeten: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Sweeten>();
}

TEST_CASE ("Airwindows Sweeten: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Sweeten>();
}

TEST_CASE ("Airwindows Sweeten: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Sweeten>();
}
