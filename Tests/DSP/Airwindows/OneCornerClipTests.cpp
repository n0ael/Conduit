/* ========================================
 *  OneCornerClipTests.cpp — DoD-Tests für den OneCornerClip-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/OneCornerClip.h"

using conduit::airwindows::OneCornerClip;

TEST_CASE ("Airwindows OneCornerClip: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<OneCornerClip>();
}

TEST_CASE ("Airwindows OneCornerClip: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<OneCornerClip>();
}

TEST_CASE ("Airwindows OneCornerClip: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<OneCornerClip>();
}
