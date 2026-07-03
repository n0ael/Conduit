/* ========================================
 *  SingleEndedTriodeTests.cpp — DoD-Tests für den SingleEndedTriode-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/SingleEndedTriode.h"

using conduit::airwindows::SingleEndedTriode;

TEST_CASE ("Airwindows SingleEndedTriode: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<SingleEndedTriode>();
}

TEST_CASE ("Airwindows SingleEndedTriode: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<SingleEndedTriode>();
}

TEST_CASE ("Airwindows SingleEndedTriode: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<SingleEndedTriode>();
}
