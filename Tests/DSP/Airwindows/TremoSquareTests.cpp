/* ========================================
 *  TremoSquareTests.cpp — DoD-Tests für den TremoSquare-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/TremoSquare.h"

using conduit::airwindows::TremoSquare;

TEST_CASE ("Airwindows TremoSquare: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<TremoSquare>();
}

TEST_CASE ("Airwindows TremoSquare: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<TremoSquare>();
}

TEST_CASE ("Airwindows TremoSquare: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<TremoSquare>();
}
