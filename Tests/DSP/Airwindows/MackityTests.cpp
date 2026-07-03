/* ========================================
 *  MackityTests.cpp — DoD-Tests für den Mackity-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Mackity.h"

using conduit::airwindows::Mackity;

TEST_CASE ("Airwindows Mackity: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Mackity>();
}

TEST_CASE ("Airwindows Mackity: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Mackity>();
}

TEST_CASE ("Airwindows Mackity: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Mackity>();
}
