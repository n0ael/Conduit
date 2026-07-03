/* ========================================
 *  DiscontapeityTests.cpp — DoD-Tests für den Discontapeity-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Discontapeity.h"

using conduit::airwindows::Discontapeity;

TEST_CASE ("Airwindows Discontapeity: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Discontapeity>();
}

TEST_CASE ("Airwindows Discontapeity: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Discontapeity>();
}

TEST_CASE ("Airwindows Discontapeity: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Discontapeity>();
}
