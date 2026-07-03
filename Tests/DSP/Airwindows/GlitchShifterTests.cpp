/* ========================================
 *  GlitchShifterTests.cpp — DoD-Tests für den GlitchShifter-Port (CLAUDE.local.md)
 * ======================================== */

#include "AirwindowsTestHelpers.h"
#include "DSP/Airwindows/Plugins/GlitchShifter.h"

using conduit::airwindows::GlitchShifter;

TEST_CASE ("Airwindows GlitchShifter: Null-Test (Silence -> Silence, Dither off)", "[airwindows][dsp]")
{
    airwindows_tests::runNullTest<GlitchShifter>();
}

TEST_CASE ("Airwindows GlitchShifter: Blockgroessen-Invarianz 64 vs. 512", "[airwindows][dsp]")
{
    airwindows_tests::runBlockSizeInvariance<GlitchShifter>();
}

TEST_CASE ("Airwindows GlitchShifter: Parameter-Sweep ohne NaN/Inf/Denormals", "[airwindows][dsp]")
{
    airwindows_tests::runParameterSweep<GlitchShifter>();
}

//==============================================================================
// Klick-Audit (Regression für den Dual-Tap-Umbau, User-Fund 03.07.2026):
// Sinuston rein, Tighten/Trim/Note werden wie von einem User-Sweep automatisiert
// (blockweise Änderungen) — der Ausgang darf dabei KEINE harten Sample-Sprünge
// zeigen. Schranke: Sinus-Steigung (~0.015) + Splice-Crossfade-Steigung
// (Tap-Differenz · 1/kMinSpliceFade ≤ ~0.06) + Duck-Steigung (1/128) plus
// Reserve. Die vor dem Fix gemessenen Klicks der WAV-Analyse lagen bei
// 0.18–0.40 — deutlich über der Schranke.
TEST_CASE ("Airwindows GlitchShifter: Klick-Audit unter Regler-Automation", "[airwindows][dsp]")
{
    auto plugin = std::make_unique<GlitchShifter>();
    plugin->prepare (48000.0);
    plugin->setParameter (GlitchShifter::kParamD, 0.0f);   // Feedback aus
    plugin->setParameter (GlitchShifter::kParamE, 1.0f);   // voll nass

    constexpr int blockSize = 64;
    constexpr int fs = 48000;
    const double phaseInc = 2.0 * 3.141592653589793238 * 220.0 / fs;
    double phase = 0.0;

    std::vector<float> ioL (blockSize), ioR (blockSize);
    std::vector<float> outHistory;   // Diagnose: kompletter Ausgang für Fehler-Dump
    outHistory.reserve (8 * fs);

    double maxJump = 0.0;
    int maxJumpAt = 0;
    double prev = 0.0;
    bool first = true;
    int sampleIndex = 0;
    bool counting = false;

    // Diagnose: Zustands-Log für alle Sprünge > 0.05 (Zustand am Blockanfang
    // VOR dem process-Aufruf des betroffenen Blocks)
    struct JumpLog { int at; double jump; int gcount, width, activeTap; double duck, pos0, pos1, xfade; };
    std::vector<JumpLog> jumpLogs;

    auto renderBlocks = [&] (int numBlocks, auto&& setParams)
    {
        for (int b = 0; b < numBlocks; ++b)
        {
            setParams (b, numBlocks);
            const JumpLog pre { 0, 0.0, plugin->getDebugGcount(), plugin->getDebugWidth(),
                                plugin->getDebugActiveTapL(), plugin->getDebugDuck(),
                                plugin->getDebugTapPosL (0), plugin->getDebugTapPosL (1),
                                plugin->getDebugXfadeL() };
            for (int i = 0; i < blockSize; ++i)
            {
                ioL[(size_t) i] = ioR[(size_t) i] = (float) (0.5 * std::sin (phase));
                phase += phaseInc;
            }
            plugin->process (ioL.data(), ioR.data(), ioL.data(), ioR.data(), blockSize);
            for (int i = 0; i < blockSize; ++i)
            {
                const double v = ioL[(size_t) i];
                outHistory.push_back (ioL[(size_t) i]);
                if (! first && counting)
                {
                    const double d = std::fabs (v - prev);
                    if (d > maxJump) { maxJump = d; maxJumpAt = sampleIndex; }
                    if (d > 0.05 && jumpLogs.size() < 30)
                    {
                        JumpLog log = pre;
                        log.at = sampleIndex;
                        log.jump = d;
                        jumpLogs.push_back (log);
                    }
                }
                prev = v;
                first = false;
                ++sampleIndex;
            }
        }
    };

    // 1 s einschwingen (Neutralstellung), noch nicht werten
    renderBlocks (fs / blockSize, [] (int, int) {});
    counting = true;

    // Tighten 0 -> 1 -> 0 über 2 s (Geometrie-Wechsel + Duck-Pfad)
    renderBlocks (2 * fs / blockSize, [&] (int b, int n)
    {
        const double t = (double) b / (double) n;
        plugin->setParameter (GlitchShifter::kParamC, (float) (t < 0.5 ? t * 2.0 : 2.0 - t * 2.0));
    });

    // Tighten hoch halten (kleine width — die Krach-Stellung des Users),
    // Trim 0 -> 1 -> 0 über 2 s (Splice-Dauerbetrieb)
    plugin->setParameter (GlitchShifter::kParamC, 0.8f);
    renderBlocks (2 * fs / blockSize, [&] (int b, int n)
    {
        const double t = (double) b / (double) n;
        plugin->setParameter (GlitchShifter::kParamB, (float) (t < 0.5 ? t * 2.0 : 2.0 - t * 2.0));
    });

    // Note hart springen lassen (härter als jeder User-Sweep)
    renderBlocks (2 * fs / blockSize, [&] (int b, int)
    {
        plugin->setParameter (GlitchShifter::kParamA, (b % 8 < 4) ? 0.9f : 0.2f);
    });

    // Diagnose bei Fehlschlag: Umgebung des größten Sprungs + Zustands-Log
    if (maxJump >= 0.08)
    {
        std::string window;
        for (int i = std::max (0, maxJumpAt - 10); i < std::min ((int) outHistory.size(), maxJumpAt + 10); ++i)
        {
            window += (i == maxJumpAt) ? " >>" : " ";
            window += std::to_string (outHistory[(size_t) i]);
        }
        UNSCOPED_INFO ("Fenster um maxJumpAt: " << window);

        for (const auto& log : jumpLogs)
            UNSCOPED_INFO ("Jump at=" << log.at << " d=" << log.jump
                << " | Blockanfang: gcount=" << log.gcount << " w=" << log.width
                << " duck=" << log.duck << " act=" << log.activeTap
                << " pos0=" << log.pos0 << " pos1=" << log.pos1
                << " xfade=" << log.xfade);
    }
    CAPTURE (maxJump, maxJumpAt);
    REQUIRE (maxJump < 0.08);
}

//==============================================================================
// Feedback-Overload-Regression (User-Fund 03.07.2026): volles Feedback ließ
// die unbegrenzte Trajektorien-Extrapolation des Originals ins Unendliche
// wachsen — der Ausgang fror als Riesen-DC ein (Stille + RMS==Peak am Meter)
// und erholte sich erst nach weitem Zurückdrehen. Erwartung nach dem Fix:
// Ausgang bleibt endlich und beschäftigt (Full-Scale-Drone erlaubt), und nach
// dem Zurückdrehen auf 0 kehrt normales Signal-Tracking zurück.
TEST_CASE ("Airwindows GlitchShifter: volles Feedback friert nicht als DC ein", "[airwindows][dsp]")
{
    auto plugin = std::make_unique<GlitchShifter>();
    plugin->prepare (48000.0);
    plugin->setParameter (GlitchShifter::kParamC, 0.6f);   // kleiner Ring: Schleife heizt schnell
    plugin->setParameter (GlitchShifter::kParamD, 1.0f);   // Feedback voll
    plugin->setParameter (GlitchShifter::kParamE, 1.0f);   // voll nass

    constexpr int blockSize = 64;
    constexpr int fs = 48000;
    const double phaseInc = 2.0 * 3.141592653589793238 * 220.0 / fs;
    double phase = 0.0;

    std::vector<float> ioL (blockSize), ioR (blockSize);

    auto renderSeconds = [&] (double seconds, float* minOut = nullptr, float* maxOut = nullptr)
    {
        float lo = 1.0e9f, hi = -1.0e9f;
        const int numBlocks = (int) (seconds * fs) / blockSize;
        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                ioL[(size_t) i] = ioR[(size_t) i] = (float) (0.5 * std::sin (phase));
                phase += phaseInc;
            }
            plugin->process (ioL.data(), ioR.data(), ioL.data(), ioR.data(), blockSize);
            for (int i = 0; i < blockSize; ++i)
            {
                REQUIRE (std::isfinite (ioL[(size_t) i]));
                lo = std::min (lo, ioL[(size_t) i]);
                hi = std::max (hi, ioL[(size_t) i]);
            }
        }
        if (minOut != nullptr) *minOut = lo;
        if (maxOut != nullptr) *maxOut = hi;
    };

    // 3 s Vollgas-Feedback: endlich bleiben, nicht als DC einfrieren
    // (im letzten Fenster muss der Ausgang noch schwingen, nicht konstant sein)
    renderSeconds (2.5);
    float lo = 0.0f, hi = 0.0f;
    renderSeconds (0.5, &lo, &hi);
    CAPTURE (lo, hi);
    REQUIRE (hi - lo > 0.01f);   // schwingt (kein eingefrorener DC-Wert)
    REQUIRE (hi <=  4.0f);       // Hard-Clip-Grenze des Moduls
    REQUIRE (lo >= -4.0f);

    // Feedback zurück auf 0: normales Tracking kehrt zurück (Pegel im
    // normalen Bereich, weiter schwingend)
    plugin->setParameter (GlitchShifter::kParamD, 0.0f);
    renderSeconds (1.5);
    renderSeconds (0.5, &lo, &hi);
    CAPTURE (lo, hi);
    REQUIRE (hi - lo > 0.01f);
    REQUIRE (hi <=  1.5f);
    REQUIRE (lo >= -1.5f);
}
