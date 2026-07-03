/* ========================================
 *  GatelopeTests.cpp — DoD-Tests für den Gatelope-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Gatelope.h"

using conduit::airwindows::Gatelope;

TEST_CASE ("Airwindows Gatelope: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Gatelope>();
}

TEST_CASE ("Airwindows Gatelope: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Gatelope>();
}

TEST_CASE ("Airwindows Gatelope: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Gatelope>();
}
