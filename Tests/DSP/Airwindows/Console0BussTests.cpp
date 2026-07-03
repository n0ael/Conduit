/* ========================================
 *  Console0BussTests.cpp — DoD-Tests für den Console0Buss-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Console0Buss.h"

using conduit::airwindows::Console0Buss;

TEST_CASE ("Airwindows Console0Buss: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Console0Buss>();
}

TEST_CASE ("Airwindows Console0Buss: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Console0Buss>();
}

TEST_CASE ("Airwindows Console0Buss: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Console0Buss>();
}
