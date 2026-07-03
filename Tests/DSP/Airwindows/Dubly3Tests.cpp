/* ========================================
 *  Dubly3Tests.cpp — DoD-Tests für den Dubly3-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Dubly3.h"

using conduit::airwindows::Dubly3;

TEST_CASE ("Airwindows Dubly3: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Dubly3>();
}

TEST_CASE ("Airwindows Dubly3: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Dubly3>();
}

TEST_CASE ("Airwindows Dubly3: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Dubly3>();
}
