# Airwindows-Portierung — Notizen

> Quelle (read-only, gitignored): `Third-Party/airwindows`, jeweils `plugins/LinuxVST/src/<Name>`
> Regeln: CLAUDE.local.md (Session-Scope) + CLAUDE.md v4.2

## Status

> Status-Konvention: „grün" erst nach tatsächlich gelaufenem Nachweis
> (Build warnings-clean + drei DoD-Tests + ASan), nie vorab.

| Plugin | Status | Parameter (id → Original-Name, Default) | Besonderheiten |
|---|---|---|---|
| Slew | **grün** (verifiziert 02.07.2026, Schritt 3: Debug `/W4 /WX` clean, 3 DoD-Tests + ASan grün) | `clamping` → "Clamping", 0.0 | Kein fpd-Dither am Ausgang (Altbestand 2011) — `setDitherEnabled` ist wirkungslos. `fpd` ist im Original **uninitialisiert** und wird nie advanced (konstanter Denormal-Guard); der Port seedet deterministisch, Verhalten sonst 1:1. |
| Spiral | **grün** (verifiziert 02.07.2026, Schritt 3: Debug `/W4 /WX` clean, 3 DoD-Tests + ASan grün) | — (0 Parameter) | Zustandslos bis auf fpd-Xorshift. |
| Density | **grün** (verifiziert 02.07.2026, Schritt 3: Debug `/W4 /WX` clean, 3 DoD-Tests + ASan grün) | `density` → "Density", 0.2 (= interne Skala 0.0) · `highpass` → "Highpass", 0.0 · `out_level` → "Out Level", 1.0 · `dry_wet` → "Dry/Wet", 1.0 | `rand()`-Seed des Originals durch deterministischen Seed ersetzt (CLAUDE.md 3.1). |

### Massen-Port 03.07.2026 (54 Plugins, alle Airwindows-Consolidated-Favoriten des Users)

Portiert in 8 parallelen Sessions (6 Batch-Agenten je 8 Plugins + 2 Reverb-
Agenten je 2 Plugins + Chamber/Galactic von Claude selbst), Quelle jeweils
`plugins/LinuxVST/src/<Name>/` (github.com/airwindows/airwindows, MIT) per
`curl` verifiziert (kein 404, keine Umbenennung nötig). Alle 54 nutzen
ausschließlich fest dimensionierte Klassenmember-Arrays — kein einziger Port
brauchte eine RT-Safety-Umstrukturierung wegen Lazy-Heap-Allokation.

**Dokumentierte Abweichungen vom reinen 1:1-Mechanik-Port (DoD-relevant):**
- **FatEQ, Isolator3, Pop2, Silken:** Original deklariert `intermediateL/R[16]`,
  indiziert aber bis `[spacing]` mit `spacing` geclamped auf `[1,16]` — bei
  `spacing==16` ein Off-by-one-Zugriff (UB im Original, bei Conduit-Zielraten
  ≤192kHz nie erreicht). Arrays auf `[17]` vergrößert — reine Sicherheitsreserve,
  identisches Verhalten im gesamten unterstützten Ratenbereich.
- **kBeyond:** `derez = 1.0/((int)(1.0/derez))` bei `Derez==0` (Parameter B im
  Sweep-Test) — `(int)`-Konvertierung von `+inf` ist UB. Guard ergänzt
  (`derez = (derez>0.0) ? 1.0/(double)(int)(1.0/derez) : 0.0`), Ergebnis für
  alle `derez>0` unverändert, für `derez==0` identisch zum beobachteten
  Selbstheilungs-Wert auf x86/SSE2 (Clamp auf 0.0005).
- **TapeDust:** Original ruft `rand()` MITTEN im DSP-Pfad auf (echter
  CLAUDE.md-3.1-Verstoß, nicht nur Seed-Init) — durch den ohnehin laufenden
  fpd-Xorshift ersetzt (Muster Flutter2's `nextmaxL`), Klangverhalten
  ununterscheidbar (beides ein 32-Bit-Uniform-Rauschen).
- **kCathedral5:** `bezF[bez_SampR] += ((inputSampleL + bezF[bez_InR]) * derezFreq)`
  liest bewusst vom LINKEN Signal für den RECHTEN Akkumulator — verifiziert
  identisch in beiden Original-Funktionen (`processReplacing`/
  `processDoubleReplacing`), also ein echter Original-Algorithmus-Zug, KEIN
  Transkriptionsfehler — unverändert übernommen (Fidelity-Vorrang).
- **GlitchShifter (User-Fund im Live-Test 03.07.2026 — GRÖSSTE Abweichung,
  Kern bewusst umgebaut):** Das Original knackst strukturell bei Regler-
  Bewegungen: (a) jeder Splice ist ein harter Lese-Positions-Sprung mit nur
  einem Ein-Sample-Blend (Knacksen nimmt mit Note/Trim-Auslenkung zu, weil
  häufiger gespliced wird), (b) jede Tighten-Bewegung ändert die Ring-
  Geometrie (`width`) bei offenem Ausgang — Schreibzeiger-Reset, Registry-
  Reset und unpassender Pufferinhalt erzeugen garantierte Klicks, punktuelle
  Fixes (Registry behalten, Position umskalieren, gcount-Modulo) reduzierten
  das nur. Nach User-Freigabe („komplett offen für tiefgreifende Änderungen")
  wurde der Wiedergabe-Kern neu aufgebaut, die Original-Splice-AUSWAHL
  (Zero-Cross-Matching mit bias) blieb erhalten: **Dual-Tap-Crossfade**
  (zwei Lese-Taps pro Kanal, Splice-Trigger mit Vorlauf `|speed|·fadeLen`,
  alter Tap spielt während des Fades weiter, xfade-Inversion hält den Mix
  beim Swap stetig, Fade-Länge `clamp(width/2, 16, 512)`, Retrigger-Guard
  16 Samples, Fade-Out-Tap friert an der Ring-Grenze ein) + **geduckter
  Geometrie-Wechsel** (Wet-Pfad ~1,3 ms auf 0, width/gcount/Tap-Positionen
  tauschen, über ~5 ms wieder hoch — aus Tighten-Klicks werden kurze
  Wet-Dips; Feedback und Splice-Historie laufen ungeduckt weiter). Bei
  neutralem Note/Trim (speed==0) entstehen keine Splices mehr (reine
  Delay-Wiedergabe, per Konstruktion klickfrei). Der bewusst erhaltene
  Glitch-Charakter: Splice-Ziele wählt weiterhin das Original-Matching,
  bei fehlenden Zero-Crossings gibt es weiterhin harte Ring-Sprünge — nur
  eben überblendet statt geknackst.
  **Iterations-Erkenntnisse aus der WAV-Klick-Analyse (Conduit-Capture des
  Users + Detektor-Skript + In-Test-Klick-Audit, alle 03.07.2026):**
  (1) Splices dürfen NUR bei abgeschlossenem Crossfade swappen (Ziel-Tap bei
  Gain 0), sonst teleportiert ein hörbarer Tap; der aktive Tap friert an der
  Ring-Grenze ein, bis der Fade fertig ist. (2) Eingefrorene Taps brauchen
  ~3 Samples Sicherheitsabstand vom Schreibkopf (Interpolations-Kernel liest
  rc+2 voraus). (3) Der Schreibzeiger-Zyklus umfasst Slot 0 (Original-Wrap
  auf 0 — ein Modulo-Wrap auf 1 ließ Slot 0 veralten: einmal pro Ring-Umlauf
  ein Klick, bei kleinem Ring Dauerkrachen). (4) Geometrie-Wechsel hinterlassen
  Epochen-Narben im Ring — Taps werden beim Wechsel auf frische Position
  direkt hinter dem Schreibkopf gesetzt (statt proportional skaliert), die
  Zero-Cross-Registry wird im stummen Moment geleert, ein Duck-Hold (512
  Samples) macht aus Tighten-Sweeps EIN sauberes Mute statt Pegel-Flattern.
  (5) Die unbegrenzte Trajektorien-Extrapolation des Originals wächst bei
  vollem Feedback ins Unendliche (DC-Lock: Stille + eingefrorene Meter,
  RMS==Peak) — hart auf ±8388352 begrenzt: volles Feedback ist jetzt ein
  stabiler Drone, der sich beim Zurückdrehen sofort fängt. Regression-Guards:
  Klick-Audit (max. Sample-Sprung < 0.08 unter Regler-Automation) +
  Feedback-DC-Lock-Test in GlitchShifterTests.cpp.
- **kWoodRoom:** `avg6L`/`avg6R` im Original deklariert, aber nie gelesen/
  geschrieben (totes Altbestand-Feld) — weggelassen, dokumentiert (Muster
  Galactics `vibML`/`vibMR`/`depthM`/`thunderL`/`thunderR`, ebenfalls tot).
- **ConsoleLABuss, ConsoleMCBuss, Stonefire:** Original interpoliert den
  Fader-Wert block-intern linear (`sampleFrames/inFramesToProcess`) — das
  Ergebnis ist bei `!= Default`-Werten dadurch von der BLOCKGRÖSSE selbst
  abhängig (Original-Verhalten, kein Portierungsfehler). Blockgrößen-
  Invarianz-Test für diese drei bewusst ausgelassen (Null-Test + Sweep bleiben).

Vollständige Parameter-Tabellen und Fundstellen je Plugin stehen im
Header-Kommentar der jeweiligen `Source/DSP/Airwindows/Plugins/<Name>.h`.

| Status | Plugins |
|---|---|
| **portiert, Build/Test/ASan-Verifikation ausstehend** | Air4, Cans, CansAW, Console0Buss, Console0Channel, ConsoleLABuss, ConsoleMCBuss, DeBess, DeBez, DeRez3, DigitalBlack, Discontapeity, Distance3, DubSub2, Dubly3, FatEQ, Flutter2, Gatelope, GlitchShifter, Hypersoft, Inflamer, Isolator3, Mackity, OneCornerClip, Parametric, PearEQ, Pockey2, PointyGuitar, Pop2, Silken, SingleEndedTriode, Smooth, SmoothEQ3, SoftGate, StoneFireComp, Stonefire, Sweeten, TakeCare, TapeDelay2, TapeDust, TapeHack2, ToneSlant, TremoSquare, Trianglizer, Tube2, Vibrato, Weight, Wider, Chamber, Galactic, VerbTiny, KBeyond, KCathedral5, KWoodRoom |

**Schritt-3-Verifikationsnachweis (isolierter Harness, MSVC 19.50 / VS 2026, C++20):**
`ConduitAirwindows` + `ConduitAirwindowsTests` standalone gebaut (`add_subdirectory` per absolutem Pfad, JUCE 8.0.4 + Catch2 v3.7.1 via FetchContent — Root-CMakeLists unangetastet). Debug: 10 Testfälle / 2565 Assertions grün, `/W4 /WX` clean. ASan (`/fsanitize=address`, Runtime-DLL im PATH): dieselben 10 Testfälle / 2565 Assertions grün, keine Reports. Je Plugin die 3 DoD-Tests (Null-Test, Blockgrößen-Invarianz 64/512, Parameter-Sweep ohne NaN/Inf/Denormals) plus 1 Registry-Test.

## Mechanische Anpassungen (gelten für alle Ports, kein DSP-Change)

- Portierungsbasis ist der `processReplacing`-Körper (float I/O, interne
  Berechnung double — die Double-Pfade der Originale bleiben erhalten,
  Konvertierung nur an den Sample-Grenzen per explizitem `(float)`-Cast,
  Warnings-clean unter `/W4 /WX`).
- VST2-Gerüst (AudioEffectX, Chunks, canDo, Programs) entfällt; Parameter-
  Metadaten als statische `ParameterInfo`-Tabellen, Laufzeitwerte als
  `std::atomic<float>` (Snapshot am Blockanfang → blockkonstant wie im
  Original).
- fpd-Dither-Add hinter `ditherOn()` (Property `ditherEnabled`, Default OFF);
  der Xorshift-Advance läuft immer — identisch zum
  `processDoubleReplacing`-Pfad der Originale.
- fpd-Seeds deterministisch statt `rand()`; die Original-Untergrenze 16386
  bleibt erhalten (`AirwindowsPlugin::prepare`).
- `math.h`-Aufrufe → `std::`-Äquivalente (`std::sin`, `std::fabs`, `std::pow`,
  `std::frexp`) — numerisch identisch.
- `*in1++;` (Deref ohne Wirkung) → `++in1;` etc.
- `juce::ScopedNoDenormals` liegt im Basis-`process()` (CLAUDE.local.md).

## Manuell nötig

*(noch keine — Kandidaten mit Latenz/Lookahead/Allokationen im Prozesspfad
hier listen statt automatisch portieren)*

## Definition of Done (Referenz)

1. Baut isoliert als Teil von `ConduitAirwindows` (C++20, Warnings clean)
2. Drei Catch2-Tests grün: Null-Test (Dither off), Blockgrößen-Invarianz
   (64 vs. 512), Parameter-Sweep ohne NaN/Inf/Denormals
3. ASan-Lauf sauber
4. Eintrag in dieser Datei
