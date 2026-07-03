/* ========================================
 *  Console0ChannelTests.cpp — DoD-Tests für den Console0Channel-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/Console0Channel.h"

using conduit::airwindows::Console0Channel;

TEST_CASE ("Airwindows Console0Channel: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<Console0Channel>();
}

TEST_CASE ("Airwindows Console0Channel: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<Console0Channel>();
}

TEST_CASE ("Airwindows Console0Channel: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<Console0Channel>();
}
