/* ========================================
 *  KBeyondTests.cpp — DoD-Tests für den kBeyond-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/KBeyond.h"

using conduit::airwindows::KBeyond;

TEST_CASE ("Airwindows KBeyond: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<KBeyond>();
}

TEST_CASE ("Airwindows KBeyond: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<KBeyond>();
}

TEST_CASE ("Airwindows KBeyond: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<KBeyond>();
}
