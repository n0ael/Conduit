/* ========================================
 *  Pop2Tests.cpp — DoD-Tests für den Pop2-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Pop2.h"

using conduit::airwindows::Pop2;

TEST_CASE ("Airwindows Pop2: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Pop2>();
}

TEST_CASE ("Airwindows Pop2: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Pop2>();
}

TEST_CASE ("Airwindows Pop2: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Pop2>();
}
