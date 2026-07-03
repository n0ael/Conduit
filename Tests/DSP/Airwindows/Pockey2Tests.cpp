/* ========================================
 *  Pockey2Tests.cpp — DoD-Tests für den Pockey2-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Pockey2.h"

using conduit::airwindows::Pockey2;

TEST_CASE ("Airwindows Pockey2: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Pockey2>();
}

TEST_CASE ("Airwindows Pockey2: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Pockey2>();
}

TEST_CASE ("Airwindows Pockey2: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Pockey2>();
}
