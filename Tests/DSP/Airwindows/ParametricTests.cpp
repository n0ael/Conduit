/* ========================================
 *  ParametricTests.cpp — DoD-Tests für den Parametric-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Parametric.h"

using conduit::airwindows::Parametric;

TEST_CASE ("Airwindows Parametric: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Parametric>();
}

TEST_CASE ("Airwindows Parametric: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Parametric>();
}

TEST_CASE ("Airwindows Parametric: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Parametric>();
}
