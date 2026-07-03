/* ========================================
 *  InflamerTests.cpp — DoD-Tests für den Inflamer-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Inflamer.h"

using conduit::airwindows::Inflamer;

TEST_CASE ("Airwindows Inflamer: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Inflamer>();
}

TEST_CASE ("Airwindows Inflamer: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Inflamer>();
}

TEST_CASE ("Airwindows Inflamer: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Inflamer>();
}
