/* ========================================
 *  ToneSlantTests.cpp — DoD-Tests für den ToneSlant-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/ToneSlant.h"

using conduit::airwindows::ToneSlant;

TEST_CASE ("Airwindows ToneSlant: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<ToneSlant>();
}

TEST_CASE ("Airwindows ToneSlant: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<ToneSlant>();
}

TEST_CASE ("Airwindows ToneSlant: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<ToneSlant>();
}
