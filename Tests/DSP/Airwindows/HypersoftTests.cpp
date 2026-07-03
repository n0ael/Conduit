/* ========================================
 *  HypersoftTests.cpp — DoD-Tests für den Hypersoft-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Hypersoft.h"

using conduit::airwindows::Hypersoft;

TEST_CASE ("Airwindows Hypersoft: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Hypersoft>();
}

TEST_CASE ("Airwindows Hypersoft: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Hypersoft>();
}

TEST_CASE ("Airwindows Hypersoft: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Hypersoft>();
}
