/* ========================================
 *  WiderTests.cpp — DoD-Tests für den Wider-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Wider.h"

using conduit::airwindows::Wider;

TEST_CASE ("Airwindows Wider: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Wider>();
}

TEST_CASE ("Airwindows Wider: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Wider>();
}

TEST_CASE ("Airwindows Wider: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Wider>();
}
