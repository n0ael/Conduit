/* ========================================
 *  KCathedral5Tests.cpp — DoD-Tests für den kCathedral5-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/KCathedral5.h"

using conduit::airwindows::KCathedral5;

TEST_CASE ("Airwindows KCathedral5: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<KCathedral5>();
}

TEST_CASE ("Airwindows KCathedral5: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<KCathedral5>();
}

TEST_CASE ("Airwindows KCathedral5: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<KCathedral5>();
}
