/* ========================================
 *  DeRez3Tests.cpp — DoD-Tests für den DeRez3-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/DeRez3.h"

using conduit::airwindows::DeRez3;

TEST_CASE ("Airwindows DeRez3: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<DeRez3>();
}

TEST_CASE ("Airwindows DeRez3: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<DeRez3>();
}

TEST_CASE ("Airwindows DeRez3: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<DeRez3>();
}
