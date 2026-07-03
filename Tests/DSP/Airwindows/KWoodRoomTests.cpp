/* ========================================
 *  KWoodRoomTests.cpp — DoD-Tests für den kWoodRoom-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/KWoodRoom.h"

using conduit::airwindows::KWoodRoom;

TEST_CASE ("Airwindows kWoodRoom: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<KWoodRoom>();
}

TEST_CASE ("Airwindows kWoodRoom: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<KWoodRoom>();
}

TEST_CASE ("Airwindows kWoodRoom: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<KWoodRoom>();
}
