/* ========================================
 *  ConsoleLABussTests.cpp — DoD-Tests für den ConsoleLABuss-Port (CLAUDE.local.md)
 *
 *  Hinweis Blockgroessen-Invarianz: das Original interpoliert den Master-
 *  Fader block-intern linear von gainA (voriger Block) nach gainB
 *  (aktueller Block) über den Bruch sampleFrames/inFramesToProcess —
 *  bei Master != 1.0 ist der Interpolationsverlauf dadurch von der
 *  Blockgröße selbst abhängig (Original-VST-Verhalten, kein Portierungs-
 *  fehler). Der Sweep-Test in runBlockSizeInvariance setzt alle Parameter
 *  auf 0.7, Master != 1.0 triggert also bewusst den blockgrößenabhängigen
 *  Zweig — dieser Test wird für ConsoleLABuss/ConsoleMCBuss NICHT
 *  bitidentisch sein. Siehe PORTING_NOTES.md / Abschlussbericht.
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/ConsoleLABuss.h"

using conduit::airwindows::ConsoleLABuss;

TEST_CASE ("Airwindows ConsoleLABuss: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<ConsoleLABuss>();
}

TEST_CASE ("Airwindows ConsoleLABuss: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<ConsoleLABuss>();
}
