/* ========================================
 *  TrianglizerTests.cpp — DoD-Tests für den Trianglizer-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Trianglizer.h"

using conduit::airwindows::Trianglizer;

TEST_CASE ("Airwindows Trianglizer: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Trianglizer>();
}

TEST_CASE ("Airwindows Trianglizer: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Trianglizer>();
}

TEST_CASE ("Airwindows Trianglizer: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Trianglizer>();
}
