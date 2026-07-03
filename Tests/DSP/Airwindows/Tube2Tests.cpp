/* ========================================
 *  Tube2Tests.cpp — DoD-Tests für den Tube2-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Tube2.h"

using conduit::airwindows::Tube2;

TEST_CASE ("Airwindows Tube2: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Tube2>();
}

TEST_CASE ("Airwindows Tube2: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Tube2>();
}

TEST_CASE ("Airwindows Tube2: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Tube2>();
}
