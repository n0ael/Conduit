/* ========================================
 *  Flutter2Tests.cpp — DoD-Tests für den Flutter2-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Flutter2.h"

using conduit::airwindows::Flutter2;

TEST_CASE ("Airwindows Flutter2: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Flutter2>();
}

TEST_CASE ("Airwindows Flutter2: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Flutter2>();
}

TEST_CASE ("Airwindows Flutter2: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Flutter2>();
}
