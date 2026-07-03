/* ========================================
 *  DubSub2Tests.cpp — DoD-Tests für den DubSub2-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/DubSub2.h"

using conduit::airwindows::DubSub2;

TEST_CASE ("Airwindows DubSub2: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<DubSub2>();
}

TEST_CASE ("Airwindows DubSub2: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<DubSub2>();
}

TEST_CASE ("Airwindows DubSub2: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<DubSub2>();
}
