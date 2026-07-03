/* ========================================
 *  StoneFireCompTests.cpp — DoD-Tests für den StoneFireComp-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/StoneFireComp.h"

using conduit::airwindows::StoneFireComp;

TEST_CASE ("Airwindows StoneFireComp: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<StoneFireComp>();
}

TEST_CASE ("Airwindows StoneFireComp: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<StoneFireComp>();
}

TEST_CASE ("Airwindows StoneFireComp: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<StoneFireComp>();
}
