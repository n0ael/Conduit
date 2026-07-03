/* ========================================
 *  VerbTinyTests.cpp — DoD-Tests für den VerbTiny-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/VerbTiny.h"

using conduit::airwindows::VerbTiny;

TEST_CASE ("Airwindows VerbTiny: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<VerbTiny>();
}

TEST_CASE ("Airwindows VerbTiny: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<VerbTiny>();
}

TEST_CASE ("Airwindows VerbTiny: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<VerbTiny>();
}
