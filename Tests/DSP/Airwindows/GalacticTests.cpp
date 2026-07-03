/* ========================================
 *  GalacticTests.cpp — DoD-Tests für den Galactic-Port (PORTING_NOTES.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Galactic.h"

using conduit::airwindows::Galactic;

TEST_CASE ("Airwindows Galactic: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Galactic>();
}

TEST_CASE ("Airwindows Galactic: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Galactic>();
}

TEST_CASE ("Airwindows Galactic: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Galactic>();
}
