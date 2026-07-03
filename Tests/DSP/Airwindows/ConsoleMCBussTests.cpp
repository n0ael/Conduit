/* ========================================
 *  ConsoleMCBussTests.cpp — DoD-Tests für den ConsoleMCBuss-Port (CLAUDE.local.md)
 *
 *  Hinweis Blockgroessen-Invarianz: identische Einschränkung wie bei
 *  ConsoleLABuss — der Master-Fader interpoliert block-intern über
 *  sampleFrames/inFramesToProcess (Original-VST-Verhalten), daher ist das
 *  Ergebnis bei Master != 1.0 blockgrößenabhängig. Kein Portierungsfehler,
 *  siehe ConsoleLABussTests.cpp / Abschlussbericht.
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/ConsoleMCBuss.h"

using conduit::airwindows::ConsoleMCBuss;

TEST_CASE ("Airwindows ConsoleMCBuss: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<ConsoleMCBuss>();
}

TEST_CASE ("Airwindows ConsoleMCBuss: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<ConsoleMCBuss>();
}
