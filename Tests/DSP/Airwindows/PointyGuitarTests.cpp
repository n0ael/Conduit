/* ========================================
 *  PointyGuitarTests.cpp — DoD-Tests für den PointyGuitar-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/PointyGuitar.h"

using conduit::airwindows::PointyGuitar;

TEST_CASE ("Airwindows PointyGuitar: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<PointyGuitar>();
}

TEST_CASE ("Airwindows PointyGuitar: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<PointyGuitar>();
}

TEST_CASE ("Airwindows PointyGuitar: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<PointyGuitar>();
}
