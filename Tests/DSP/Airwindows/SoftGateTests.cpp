/* ========================================
 *  SoftGateTests.cpp — DoD-Tests für den SoftGate-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/SoftGate.h"

using conduit::airwindows::SoftGate;

TEST_CASE ("Airwindows SoftGate: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<SoftGate>();
}

TEST_CASE ("Airwindows SoftGate: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<SoftGate>();
}

TEST_CASE ("Airwindows SoftGate: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<SoftGate>();
}
