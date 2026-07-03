/* ========================================
 *  DigitalBlackTests.cpp — DoD-Tests für den DigitalBlack-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/DigitalBlack.h"

using conduit::airwindows::DigitalBlack;

TEST_CASE ("Airwindows DigitalBlack: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<DigitalBlack>();
}

TEST_CASE ("Airwindows DigitalBlack: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<DigitalBlack>();
}

TEST_CASE ("Airwindows DigitalBlack: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<DigitalBlack>();
}
