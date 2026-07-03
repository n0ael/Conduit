/* ========================================
 *  GlitchShifter - GlitchShifter.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/GlitchShifter.
 *  Zero-Cross-Registry, Air-Sektion, Feedback-Pfad und die 24-Bit-Integer-
 *  Skala (truncToInt32-Kette) sind 1:1 aus dem Original übernommen.
 *
 *  BEWUSST UMGEBAUT (User-Entscheidung 03.07.2026, Details im Header):
 *  Dual-Tap-Splice-Crossfade + gedeckter Geometrie-Wechsel statt der
 *  klickenden Original-Sprünge. Die Original-Splice-AUSWAHL (welcher
 *  Zero-Cross-Punkt passt am besten zur aktuellen Wellenform) bleibt
 *  unverändert — nur der ÜBERGANG dorthin ist jetzt ein echter Crossfade.
 * ======================================== */

#include "DSP/Airwindows/Plugins/GlitchShifter.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "note",    "Note",    0.5f },
        { "trim",    "Trim",    0.5f },
        { "tighten", "Tighten", 0.5f },
        { "feedbck", "Feedbck", 0.0f },
        { "dry_wet", "Dry/Wet", 0.5f },
    };

    // Original castet Zwischenwerte auf VstInt32 (32-Bit-Trunkierung).
    inline double truncToInt32 (double value) noexcept
    {
        return (double) (std::int32_t) value;
    }
}

GlitchShifter::GlitchShifter() noexcept
    : AirwindowsPlugin ("glitchshifter", "GlitchShifter", kParameters)
{
}

void GlitchShifter::resetState() noexcept
{
    for (auto& v : pL) v = 0.0;
    for (auto& v : pR) v = 0.0;
    for (int count = 0; count < kCrossSize; ++count)
    {
        offsetL[count] = 0.0; pastzeroL[count] = 0.0; previousL[count] = 0.0; thirdL[count] = 0.0; fourthL[count] = 0.0;
        offsetR[count] = 0.0; pastzeroR[count] = 0.0; previousR[count] = 0.0; thirdR[count] = 0.0; fourthR[count] = 0.0;
    }

    crossesL = 0;
    realzeroesL = 0;
    tempL = 0.0;
    lasttempL = 0.0;
    thirdtempL = 0.0;
    fourthtempL = 0.0;
    sincezerocrossL = 0.0;
    airPrevL = 0.0;
    airEvenL = 0.0;
    airOddL = 0.0;
    airFactorL = 0.0;
    tapPosL[0] = 0.0;
    tapPosL[1] = 0.0;
    activeTapL = 0;
    xfadeL = 1.0;
    swapGuardL = 0;

    crossesR = 0;
    realzeroesR = 0;
    tempR = 0.0;
    lasttempR = 0.0;
    thirdtempR = 0.0;
    fourthtempR = 0.0;
    sincezerocrossR = 0.0;
    airPrevR = 0.0;
    airEvenR = 0.0;
    airOddR = 0.0;
    airFactorR = 0.0;
    tapPosR[0] = 0.0;
    tapPosR[1] = 0.0;
    activeTapR = 0;
    xfadeR = 1.0;
    swapGuardR = 0;

    gcount = 0;
    flip = false;
    currentWidth = 0;
    duckGain = 1.0;
    duckHold = 0;
}

void GlitchShifter::processStereo (const float* in1, const float* in2,
                                   float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);

    int note = (int) (A * 24.999) - 12;
    double trim = (B * 2.0) - 1.0;
    double speed = ((1.0 / 12.0) * note) + trim;
    if (speed < 0) speed *= 0.5;
    //include sanity check- maximum pitch lowering cannot be to 0 hz.
    const int targetWidth = (int) (65536 - ((1 - std::pow (1 - C, 2)) * 65530.0));
    double bias = std::pow (C, 3);
    double feedback = D / 1.5;
    double wet = E;

    if (currentWidth <= 0)
        currentWidth = targetWidth;   // erster Block nach reset: ohne Duck übernehmen

    // Crossfade-Länge folgt der Ringgröße. Der Splice wird exakt an der
    // Ring-Grenze ausgelöst (Original-Kadenz); der auslaufende Tap friert
    // dort ein statt zu springen — an der Dry-Tap-Grenze (pos 0) liest ein
    // eingefrorener Tap schlicht das Live-Signal weiter, stetig per
    // Konstruktion. (Ein früherer Zwischenstand zog den Trigger um die
    // Fade-Strecke vor — bei hohen Speeds wählte die Registry dann oft
    // Positionen ohne Restlaufzeit und der Fallback sprang ungeblendet:
    // Dauer-Krachen, User-Fund 03.07.2026.)
    int spliceFade = currentWidth / 4;
    if (spliceFade < kMinSpliceFade) spliceFade = kMinSpliceFade;
    if (spliceFade > kMaxSpliceFade) spliceFade = kMaxSpliceFade;
    const double xfadeStep = 1.0 / (double) spliceFade;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        airFactorL = airPrevL - inputSampleL;
        if (flip) { airEvenL += airFactorL; airOddL -= airFactorL; airFactorL = airEvenL; }
        else { airOddL += airFactorL; airEvenL -= airFactorL; airFactorL = airOddL; }
        airOddL = (airOddL - ((airOddL - airEvenL) / 256.0)) / 1.0001;
        airEvenL = (airEvenL - ((airEvenL - airOddL) / 256.0)) / 1.0001;
        airPrevL = inputSampleL;
        inputSampleL += airFactorL;

        airFactorR = airPrevR - inputSampleR;
        if (flip) { airEvenR += airFactorR; airOddR -= airFactorR; airFactorR = airEvenR; }
        else { airOddR += airFactorR; airEvenR -= airFactorR; airFactorR = airOddR; }
        airOddR = (airOddR - ((airOddR - airEvenR) / 256.0)) / 1.0001;
        airEvenR = (airEvenR - ((airEvenR - airOddR) / 256.0)) / 1.0001;
        airPrevR = inputSampleR;
        inputSampleR += airFactorR;

        flip = !flip;
        //air, compensates for loss of highs of interpolation

        // --- Tighten-/width-Wechsel: Geometrie (Ringgröße, Schreibzeiger,
        // Tap-Positionen) NUR bei stummgeschaltetem Wet-Pfad tauschen —
        // ein Geometrie-Sprung bei offenem Ausgang knackst prinzipbedingt
        // (Schreibzeiger, Lese-Adressen und Pufferinhalt passen nicht mehr
        // zusammen). Solange ein Wechsel ansteht, bleibt der Duck unten.
        if (currentWidth != targetWidth)
        {
            duckGain -= kDuckDownStep;
            if (duckGain <= 0.0)
            {
                duckGain = 0.0;
                currentWidth = targetWidth;
                // In den neuen Zyklus [0..w] einpassen (w+1 Slots)
                while (gcount > currentWidth) gcount -= (currentWidth + 1);
                if (gcount < 0) gcount = 0;

                // Taps DIREKT HINTER den Schreibkopf setzen statt proportional
                // umzuskalieren: nach einem Geometrie-Wechsel ist der Ring-
                // Inhalt ein Flickwerk verschiedener Zeit-Epochen (der Kopf-
                // Teleport hinterlässt Narben). Nahe des Kopfs ist der Inhalt
                // garantiert frisch UND epochenrein — und da der Kopf mit
                // 1 Sample/Sample schreibt, während die Taps höchstens mit
                // |speed| ≤ 1 zurückfallen, erreichen sie alte Narben nie
                // (WAV-/Audit-Fund 03.07.2026: Phasensprünge durch Lesen
                // über Epochen-Nähte, bis zu w Samples NACH dem Wechsel).
                {
                    const double freshPos = (currentWidth > 8) ? 3.0 : (double) currentWidth * 0.5;
                    tapPosL[0] = tapPosL[1] = freshPos;
                    tapPosR[0] = tapPosR[1] = freshPos;
                    xfadeL = 1.0;
                    xfadeR = 1.0;
                }

                // Zero-Cross-Registry leeren: die gespeicherten offsets sind
                // Adressen der ALTEN Epoche — ein Splice dorthin läse Narben.
                // (Das wollte der Original-Reset bei width-Wechsel erreichen —
                // nur eben ungemutet und damit selbst klickend; hier passiert
                // er im stummen Duck-Moment.)
                crossesL = 0; realzeroesL = 0; sincezerocrossL = 0.0;
                crossesR = 0; realzeroesR = 0; sincezerocrossR = 0.0;

                // Schatten-Naht der NEUEN Ringgröße sofort auffrischen (die
                // Interpolation liest rc+1/rc+2 über die Naht aus p[w..w+2]).
                pL[(size_t) currentWidth]     = pL[0];
                pL[(size_t) currentWidth + 1] = pL[1];
                pL[(size_t) currentWidth + 2] = pL[2];
                pR[(size_t) currentWidth]     = pR[0];
                pR[(size_t) currentWidth + 1] = pR[1];
                pR[(size_t) currentWidth + 2] = pR[2];

                duckHold = kDuckHoldSamples;
            }
        }
        else if (duckHold > 0)
        {
            --duckHold;   // nach dem Wechsel kurz unten bleiben (Sweep-Mute)
        }
        else if (duckGain < 1.0)
        {
            duckGain += kDuckUpStep;
            if (duckGain > 1.0) duckGain = 1.0;
        }
        const int w = currentWidth;

        // Sicherheits-Korridor für die Lese-Taps: der Interpolations-Kernel
        // liest rc..rc+2, bei pos < ~3 also ÜBER den Schreibkopf hinweg in
        // noch nicht geschriebene (einen Ring-Zyklus alte) Samples — ein
        // dauerhaft dort geparkter (eingefrorener) Tap klickt. Gleiches am
        // oberen Rand (pos ≈ w wrappt auf den Schreibkopf zurück).
        const double posLo = (w > 8) ? 3.0 : (double) w * 0.5;
        const double posHi = (w > 8) ? (double) w - 3.0 : (double) w * 0.5;

        // Original-Wrap auf 0: der Ring hat w+1 Slots [0..w], Slot 0 gehört
        // zum Schreibzyklus. (Ein früherer Zwischenstand wrappte modulo w
        // auf 1 — Slot 0 wurde dann NIE mehr beschrieben und jeder Lese-Tap
        // erwischte dort einmal pro Ring-Zyklus uralte Daten: die "seltenen
        // Knackser bei Neutralstellung" bzw. das Dauerkrachen bei kleinem
        // Ring, WAV-Analyse 03.07.2026.) Der Hart-Reset ist hier sicher,
        // weil gcount nur um 1 wächst; width-Wechsel laufen im Duck (oben).
        gcount++;
        if (gcount < 0 || gcount > w) { gcount = 0; }
        int count = gcount;
        int countone = count - 1;
        int counttwo = count - 2;
        while (countone < 0) { countone += w; }
        while (counttwo < 0) { counttwo += w; }
        //we are tracking most recent samples and must SUBTRACT.
        //this is a wrap on the overall buffers, so count, one and two are also common to both channels

        // Feedback wird mit duckGain skaliert: während eines Geometrie-
        // Wechsels springt der interne Tap-Mix (unhörbar, Wet ist geduckt) —
        // ungeduckt würde dieser Sprung über den Feedback-Pfad in den Puffer
        // AUFGENOMMEN und nach dem Duck hörbar abgespielt (Rest-Klick-Quelle
        // bei Feedbck > 0, User-Fund).
        pL[(size_t) (count + w)] = pL[(size_t) count] = truncToInt32 ((inputSampleL * 8388352.0) + (lasttempL * feedback * duckGain));
        pR[(size_t) (count + w)] = pR[(size_t) count] = truncToInt32 ((inputSampleR * 8388352.0) + (lasttempR * feedback * duckGain));
        //double buffer -8388352 to 8388352 is equal to 24 bit linear space

        if ((pL[(size_t) countone] > 0 && pL[(size_t) count] < 0) || (pL[(size_t) countone] < 0 && pL[(size_t) count] > 0)) //source crossed zero
        {
            crossesL++;
            realzeroesL++;
            if (crossesL > 256) { crossesL = 0; } //wrap crosses to keep adding new crosses
            if (realzeroesL > 256) { realzeroesL = 256; } //don't wrap realzeroes, full buffer, use all
            offsetL[(size_t) crossesL] = count;
            pastzeroL[(size_t) crossesL] = pL[(size_t) count];
            previousL[(size_t) crossesL] = pL[(size_t) countone];
            thirdL[(size_t) crossesL] = pL[(size_t) counttwo];
        } //we just put in a source zero cross in the registry

        if ((pR[(size_t) countone] > 0 && pR[(size_t) count] < 0) || (pR[(size_t) countone] < 0 && pR[(size_t) count] > 0)) //source crossed zero
        {
            crossesR++;
            realzeroesR++;
            if (crossesR > 256) { crossesR = 0; }
            if (realzeroesR > 256) { realzeroesR = 256; }
            offsetR[(size_t) crossesR] = count;
            pastzeroR[(size_t) crossesR] = pR[(size_t) count];
            previousR[(size_t) crossesR] = pR[(size_t) countone];
            thirdR[(size_t) crossesR] = pR[(size_t) counttwo];
        } //we just put in a source zero cross in the registry

        // ============================= LINKS =============================
        {
            // Beide Taps bewegen; am Sicherheits-Korridor FRIEREN sie ein
            // statt zu springen (nahe des Dry-Taps = quasi Live-Signal,
            // stetig). Es gibt bewusst KEINEN ungeblendeten Wrap-Pfad.
            for (int t = 0; t < 2; ++t)
            {
                tapPosL[(size_t) t] -= speed;
                if (tapPosL[(size_t) t] < posLo) tapPosL[(size_t) t] = posLo;
                else if (tapPosL[(size_t) t] > posHi) tapPosL[(size_t) t] = posHi;
            }
            const int inactive = 1 - activeTapL;

            if (swapGuardL > 0) --swapGuardL;

            // Splice, sobald der aktive Tap an seiner Ziel-Grenze angefroren
            // ist (Original-Kadenz) — aber NUR wenn der vorige Fade fertig
            // ist (xfade == 1): der Ziel-Tap steht dann bei Gain 0 und darf
            // klickfrei teleportieren. Ein mid-fade repositionierter Tap bei
            // Gain > 0 war die Haupt-Klick-Quelle der WAV-Analyse (03.07.).
            // Bei speed == 0 bewegt sich nichts und es gibt nie einen Splice.
            const bool wantSplice = (speed > 0.0 && tapPosL[(size_t) activeTapL] <= posLo)
                                 || (speed < 0.0 && tapPosL[(size_t) activeTapL] >= posHi);
            if (wantSplice && swapGuardL == 0 && xfadeL >= 1.0)
            {
                double newPos;
                if (realzeroesL > 0)
                {
                    //we have zero crosses in the bin — Original-Auswahl:
                    double diff = 99999999.0;
                    int best = 0;
                    for (int scan = (realzeroesL - 1); scan >= 0; scan--)
                    {
                        int scanone = scan + crossesL;
                        if (scanone > 256) { scanone -= 256; }
                        //try to track the real most recent ones more closely
                        double howdiff = (tempL - pastzeroL[(size_t) scanone]) + (lasttempL - previousL[(size_t) scanone])
                                       + (thirdtempL - thirdL[(size_t) scanone]) + (fourthtempL - fourthL[(size_t) scanone]);
                        howdiff -= ((double) scan * bias);
                        //try to bias in favor of more recent crosses
                        if (howdiff < diff) { diff = howdiff; best = scanone; }
                    } //now we have 'best' as the closest match to the current rate of zero cross and positioning- a splice.
                    newPos = offsetL[(size_t) best] - sincezerocrossL;
                }
                else
                {
                    //no crosses in the bin- glitch speeds: harter Ring-Sprung
                    newPos = tapPosL[(size_t) activeTapL] + ((speed > 0.0) ? (double) w : (double) -w);
                }
                // In den Ring wrappen (Ring-Sprung ist Original-Vokabular) —
                // garantiert Restlaufzeit statt sofortigem Re-Splice —
                // und in den Sicherheits-Korridor klemmen.
                while (newPos < 0.0) newPos += (double) w;
                while (newPos > (double) w) newPos -= (double) w;
                if (newPos < posLo) newPos = posLo;
                if (newPos > posHi) newPos = posHi;
                crossesL = 0;
                realzeroesL = 0;

                // Dual-Tap-Swap: der Ziel-Tap steht bei Gain 0 (Fade fertig,
                // Gate oben) und übernimmt die Splice-Position klickfrei; der
                // alte bleibt (angefroren) hörbar und blendet aus.
                tapPosL[(size_t) inactive] = newPos;
                activeTapL = inactive;
                xfadeL = 0.0;
                swapGuardL = kSwapGuardSamples;
            }

            // Beide Taps lesen (Original-Interpolation inkl. Integer-Trunkierung)
            double tapOut[2];
            for (int t = 0; t < 2; ++t)
            {
                const double pos = tapPosL[(size_t) t];
                int rc = gcount - (int) std::floor (pos);
                while (rc < 0) { rc += w; }
                while (rc > w) { rc -= w; }
                const double frac = pos - std::floor (pos);
                double v = 0.0;
                v += truncToInt32 (pL[(size_t) rc] * (1 - frac)); //less as value moves away from .0
                v += pL[(size_t) (rc + 1)]; //we can assume always using this in one way or another?
                v += truncToInt32 (pL[(size_t) (rc + 2)] * frac); //greater as value moves away from .0
                v -= truncToInt32 (((pL[(size_t) rc] - pL[(size_t) (rc + 1)]) - (pL[(size_t) (rc + 1)] - pL[(size_t) (rc + 2)])) / 50); //interpolation hacks 'r us
                v = truncToInt32 (v / 2); //gotta make temp be the same level scale as buffer
                tapOut[t] = v;
            }
            tempL = (tapOut[activeTapL] * xfadeL) + (tapOut[1 - activeTapL] * (1.0 - xfadeL));
            if (xfadeL < 1.0) { xfadeL += xfadeStep; if (xfadeL > 1.0) xfadeL = 1.0; }

            if (std::fabs (tempL) > 8388352.0) { tempL = (lasttempL + (lasttempL - thirdtempL)); }
            //kill ticks of bad buffer mojo by sticking with the trajectory. Ugly hack *shrug*
            // Extrapolation hart auf die 24-Bit-Skala begrenzen: bei hohem
            // Feedback überschreitet die Schleife die Grenze dauerhaft, die
            // unbegrenzte Original-Extrapolation füttert sich dann selbst und
            // wächst ins Unendliche — Ausgang friert als Riesen-DC ein
            // (Stille + eingefrorene Meter mit RMS==Peak, User-Fund
            // 03.07.2026). Begrenzt wird volles Feedback stattdessen ein
            // stabiler Full-Scale-Drone, der sich beim Zurückdrehen sofort
            // wieder fängt.
            if (tempL > 8388352.0) tempL = 8388352.0;
            else if (tempL < -8388352.0) tempL = -8388352.0;

            sincezerocrossL++;
            if (sincezerocrossL < 0 || sincezerocrossL > w) { sincezerocrossL = 0; } //just a sanity check
            if ((lasttempL > 0 && tempL < 0) || (lasttempL < 0 && tempL > 0)) //delay tap crossed zero
            {
                sincezerocrossL = 0;
            } //we just restarted counting from the delay tap zero cross
        }

        // ============================= RECHTS =============================
        {
            for (int t = 0; t < 2; ++t)
            {
                tapPosR[(size_t) t] -= speed;
                if (tapPosR[(size_t) t] < posLo) tapPosR[(size_t) t] = posLo;
                else if (tapPosR[(size_t) t] > posHi) tapPosR[(size_t) t] = posHi;
            }
            const int inactive = 1 - activeTapR;

            if (swapGuardR > 0) --swapGuardR;

            const bool wantSplice = (speed > 0.0 && tapPosR[(size_t) activeTapR] <= posLo)
                                 || (speed < 0.0 && tapPosR[(size_t) activeTapR] >= posHi);
            if (wantSplice && swapGuardR == 0 && xfadeR >= 1.0)
            {
                double newPos;
                if (realzeroesR > 0)
                {
                    double diff = 99999999.0;
                    int best = 0;
                    for (int scan = (realzeroesR - 1); scan >= 0; scan--)
                    {
                        int scanone = scan + crossesR;
                        if (scanone > 256) { scanone -= 256; }
                        double howdiff = (tempR - pastzeroR[(size_t) scanone]) + (lasttempR - previousR[(size_t) scanone])
                                       + (thirdtempR - thirdR[(size_t) scanone]) + (fourthtempR - fourthR[(size_t) scanone]);
                        howdiff -= ((double) scan * bias);
                        if (howdiff < diff) { diff = howdiff; best = scanone; }
                    }
                    newPos = offsetR[(size_t) best] - sincezerocrossR;
                }
                else
                {
                    newPos = tapPosR[(size_t) activeTapR] + ((speed > 0.0) ? (double) w : (double) -w);
                }
                while (newPos < 0.0) newPos += (double) w;
                while (newPos > (double) w) newPos -= (double) w;
                if (newPos < posLo) newPos = posLo;
                if (newPos > posHi) newPos = posHi;
                crossesR = 0;
                realzeroesR = 0;

                tapPosR[(size_t) inactive] = newPos;
                activeTapR = inactive;
                xfadeR = 0.0;
                swapGuardR = kSwapGuardSamples;
            }

            double tapOut[2];
            for (int t = 0; t < 2; ++t)
            {
                const double pos = tapPosR[(size_t) t];
                int rc = gcount - (int) std::floor (pos);
                while (rc < 0) { rc += w; }
                while (rc > w) { rc -= w; }
                const double frac = pos - std::floor (pos);
                double v = 0.0;
                v += truncToInt32 (pR[(size_t) rc] * (1 - frac));
                v += pR[(size_t) (rc + 1)];
                v += truncToInt32 (pR[(size_t) (rc + 2)] * frac);
                v -= truncToInt32 (((pR[(size_t) rc] - pR[(size_t) (rc + 1)]) - (pR[(size_t) (rc + 1)] - pR[(size_t) (rc + 2)])) / 50);
                v = truncToInt32 (v / 2);
                tapOut[t] = v;
            }
            tempR = (tapOut[activeTapR] * xfadeR) + (tapOut[1 - activeTapR] * (1.0 - xfadeR));
            if (xfadeR < 1.0) { xfadeR += xfadeStep; if (xfadeR > 1.0) xfadeR = 1.0; }

            if (std::fabs (tempR) > 8388352.0) { tempR = (lasttempR + (lasttempR - thirdtempR)); }
            // Begrenzung gegen den Feedback-DC-Lock — siehe linken Kanal.
            if (tempR > 8388352.0) tempR = 8388352.0;
            else if (tempR < -8388352.0) tempR = -8388352.0;

            sincezerocrossR++;
            if (sincezerocrossR < 0 || sincezerocrossR > w) { sincezerocrossR = 0; }
            if ((lasttempR > 0 && tempR < 0) || (lasttempR < 0 && tempR > 0))
            {
                sincezerocrossR = 0;
            }
        }

        fourthtempL = thirdtempL;
        thirdtempL = lasttempL;
        lasttempL = tempL;

        fourthtempR = thirdtempR;
        thirdtempR = lasttempR;
        lasttempR = tempR;

        // duckGain wirkt NUR auf den Wet-Anteil des Ausgangs — Feedback und
        // Splice-Historie (lasttemp/thirdtemp) laufen ungeduckt weiter, damit
        // der Puffer-Inhalt über einen Geometrie-Wechsel konsistent bleibt.
        inputSampleL = (drySampleL * (1 - wet)) + ((tempL / (8388352.0)) * wet * duckGain);
        if (inputSampleL > 4.0) inputSampleL = 4.0;
        if (inputSampleL < -4.0) inputSampleL = -4.0;

        inputSampleR = (drySampleR * (1 - wet)) + ((tempR / (8388352.0)) * wet * duckGain);
        if (inputSampleR > 4.0) inputSampleR = 4.0;
        if (inputSampleR < -4.0) inputSampleR = -4.0;
        //this plugin can throw insane outputs so we'll put in a hard clip

        //begin 32 bit stereo floating point dither
        if (ditherOn())
        {
            int expon;
            std::frexp ((float) inputSampleL, &expon);
            fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
            inputSampleL += (double) ((double (fpdL) - std::uint32_t (0x7fffffff)) * 5.5e-36l * std::pow (2, expon + 62));
            std::frexp ((float) inputSampleR, &expon);
            fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
            inputSampleR += (double) ((double (fpdR) - std::uint32_t (0x7fffffff)) * 5.5e-36l * std::pow (2, expon + 62));
        }
        else
        {
            fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
            fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
        }
        //end 32 bit stereo floating point dither

        *out1 = (float) inputSampleL;
        *out2 = (float) inputSampleR;

        ++in1;
        ++in2;
        ++out1;
        ++out2;
    }
}

} // namespace conduit::airwindows
