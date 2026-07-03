/* ========================================
 *  Air4Tests.cpp — DoD-Tests für den Air4-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Air4.h"

using conduit::airwindows::Air4;

TEST_CASE ("Airwindows Air4: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Air4>();
}

TEST_CASE ("Airwindows Air4: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Air4>();
}

TEST_CASE ("Airwindows Air4: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Air4>();
}
