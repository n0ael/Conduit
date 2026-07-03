/* ========================================
 *  ChamberTests.cpp — DoD-Tests für den Chamber-Port (PORTING_NOTES.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Chamber.h"

using conduit::airwindows::Chamber;

TEST_CASE ("Airwindows Chamber: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Chamber>();
}

TEST_CASE ("Airwindows Chamber: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Chamber>();
}

TEST_CASE ("Airwindows Chamber: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Chamber>();
}
