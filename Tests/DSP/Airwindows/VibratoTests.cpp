/* ========================================
 *  VibratoTests.cpp — DoD-Tests für den Vibrato-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Vibrato.h"

using conduit::airwindows::Vibrato;

TEST_CASE ("Airwindows Vibrato: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Vibrato>();
}

TEST_CASE ("Airwindows Vibrato: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Vibrato>();
}

TEST_CASE ("Airwindows Vibrato: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Vibrato>();
}
