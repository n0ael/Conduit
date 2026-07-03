/* ========================================
 *  SmoothTests.cpp — DoD-Tests für den Smooth-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Smooth.h"

using conduit::airwindows::Smooth;

TEST_CASE ("Airwindows Smooth: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Smooth>();
}

TEST_CASE ("Airwindows Smooth: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Smooth>();
}

TEST_CASE ("Airwindows Smooth: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Smooth>();
}
