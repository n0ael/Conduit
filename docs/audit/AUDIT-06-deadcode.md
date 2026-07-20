# AUDIT-06 — Dead-Code-/Duplikations-Audit

> Read-only-Suche nach totem Code, verwaisten Dateien und duplizierter
> Logik · Durchgeführt: 20.07.2026 · Branch: master (b1f998c) ·
> Referenz: CLAUDE.md v5.6 (Fußzeilen-Check bestanden) ·
> Vorgänger: AUDIT-01 (RT-Safety), AUDIT-02 (Threading), AUDIT-03
> (Konformität — dessen Phase-1.2-Inventur hier wiederverwendet und
> stichprobenartig neu verifiziert)
>
> Pflichtlektüre erfüllt: CLAUDE.md §12 (Out-of-Scope/v1-Referenzen),
> §13.5 (Projektstruktur). Scope: Source/ + Tests/ + beide CMakeLists;
> build/, build-asan/ und _deps ausgeklammert.

---

## 1. Zusammenfassung

Der Baum ist für seine Größe (824 .h/.cpp-Dateien, 476 Übersetzungs-
einheiten) bemerkenswert sauber:

- **Datei-Ebene: 0 verwaiste Dateien.** Alle 476 .cpp sind in den
  CMake-Targets registriert (Neuabgleich deckungsgleich mit AUDIT-03
  D1), 0 CMake-Einträge ohne Datei, und **jeder der 348 Header wird
  von mindestens einer anderen Datei inkludiert**.
- **Klassen-Ebene: 2 tote UI-Klassen.** `CaptureAllButton` und
  `LinkAudioStatusBadge` werden kompiliert und gelinkt, aber seit
  Anfang Juli 2026 **nirgends mehr instanziiert** (Abschnitt 2.2) —
  die einzigen echten Löschkandidaten dieses Audits.
- **0 auskommentierte Codeblöcke > 10 Zeilen.** Die vier
  Heuristik-Treffer sind sämtlich Doku-Prosa (Abschnitt 3).
- **Duplikation konzentriert sich auf 4 lohnende Util-Kandidaten**
  (Meter-Ballistik, dB-Normierung, Long-Press-Geste, MM:SS-Formatter)
  plus 2 dokumentierte Negativ-Befunde (LCG und DSP-Smoothing sind
  bereits zentral — nicht „extrahieren") — Abschnitt 4.
- **TODO/FIXME/HACK: 18 Treffer, ausnahmslos `TODO(design)`** —
  bewusste, dem User zugewiesene Design-Entscheidungen, kein einziges
  technisches FIXME/HACK (Abschnitt 5).
- **0 Referenzen auf `Conduit_Alpha_v2`/alte Remote-URLs** in aktiven
  Codepfaden (Source/, Tests/, beide CMakeLists; case-insensitive).
  Die einzigen GitHub-URLs im Code sind die legitimen
  Airwindows-Quellenangaben (`github.com/airwindows/airwindows`) in
  den Port-Headern.
- **Keine v1-Referenzbestände in diesem Repo:** das in CLAUDE.md §11
  genannte `EncoderEngines.hpp` (v1-Port als Referenz) liegt NICHT im
  Arbeitsbaum — es gibt hier nichts als Kategorie (b) zu schützen.

---

## 2. Verwaiste Dateien mit Klassifikation

Klassifikation: (a) toter Code · (b) bewusste v1-/Referenz-Bestände
(§11/§12) · (c) in Arbeit befindlich.

### 2.1 Build-Ebene (Phase 1.1 — Abgleich mit AUDIT-03)

Neuabgleich Platte ↔ CMake (Root-CMakeLists + `Source/DSP/Airwindows/
CMakeLists.txt` inkl. `${CONDUIT_AIRWINDOWS_TESTS_DIR}`-Auflösung):

| Prüfung | Ergebnis |
|---|---|
| .cpp auf Platte, nicht in target_sources | **0** (476/476) |
| CMake-Eintrag ohne Datei auf Platte | **0** |
| Header, die von KEINER anderen Datei inkludiert werden | **0** von 348 |
| Header nur von Tests inkludiert (Produktivcode ohne Produktiv-Nutzer) | **0** |

Die 79 in AUDIT-03 D1 gelisteten Header fehlen nur in den rein
IDE-relevanten Header-Listen der target_sources — das ist eine
Pflege-Lücke, **kein** Dead-Code-Befund, und bleibt Folgeauftrag von
AUDIT-03.

Einzige Nicht-C++-Datei unter Source/:
`Source/DSP/Airwindows/PORTING_NOTES.md` — aktiv von Rule `fx-chassis`
referenziert, kein Befund.

### 2.2 Klassen-Ebene: kompiliert, aber nie instanziiert — Klasse (a)

Methodik: Header, die innerhalb von Source/ ausschließlich von der
eigenen .cpp inkludiert werden, auf externe Verwendung der Klasse
geprüft (Instanziierung braucht den vollständigen Typ).

**(a) `Source/UI/CaptureAllButton.{h,cpp}` — toter Code.**
Außerhalb der eigenen Dateien existiert nur noch eine
Kommentar-Erwähnung (`Source/Core/Capture/CaptureService.h:299`).
Historie: angelegt mit der Capture-Toolbar (ea649ce, 12.06.2026);
die letzte Include-Nutzung verschwand mit **8467464 (02.07.2026)**,
als die TransportBar die alte Toolbar ersetzte. Seitdem wird die
Klasse in beiden Targets (Conduit + ConduitTests) mitkompiliert, aber
nie konstruiert; Tests existieren keine. Randnotiz: die Roadmap
(CLAUDE.md §11, Mixer-Page) sagt „Capture-Buttons wandern dorthin" —
falls diese Klasse dafür bewusst vorgehalten wird, wäre das bei der
Freigabe-Entscheidung zu klären; im jetzigen Zustand (ungenutzt,
ungetestet, seit 18 Tagen ohne Konsument) ist sie toter Code.

**(a) `Source/UI/LinkAudioStatusBadge.{h,cpp}` — toter Code.**
Außerhalb der eigenen Dateien nur zwei Kommentar-Erwähnungen als
„Muster" (`Source/UI/FxModulePanel.h:201`,
`Source/UI/LinkAudioSendPanel.h:29`). Historie: angelegt mit dem
Link-Audio-Badge (1a3ff40); die letzte Include-Nutzung verschwand mit
**105eee5 (01.07.2026)**, als das LinkAudioSendPanel (+ Anlege-Dialog)
den Badge ersetzte. Kompiliert in beiden Targets, keine Tests, keine
Konstruktion. Bei Löschung wären die zwei „Muster"-Kommentare auf das
Panel umzuformulieren.

**Scheinbefund (kein Dead Code): `Source/UI/TouchLivePage/
TouchLiveEq8Panel.{h,cpp}`.** Der Header wird ebenfalls nur von der
eigenen .cpp inkludiert, die Klasse lebt aber über die
Bespoke-Registry: `createBespokePanel()` ist in
`TouchLiveBespokePanel.h:47` deklariert und in
`TouchLiveEq8Panel.cpp:1308` definiert; `TouchLiveDeviceView.cpp:410`
ruft sie auf, `Tests/UI/TouchLivePageTests.cpp:573 ff.` testet das
Panel. Hier dokumentiert, damit künftige Audits den Fund nicht
wiederholen.

### 2.3 Klasse (b) — bewusste Referenz-Bestände

**Keine vorhanden.** `EncoderEngines.hpp` (§11, v1-Port ES-5/ES-4)
liegt nicht in diesem Repo (vollständige Suche nach `*Encoder*`
liefert nur `MpeEncoder.{h,cpp}` + Test — aktiver Code). Die 58
Airwindows-Ports unter `Source/DSP/Airwindows/Plugins/` sind kein
Referenz-Bestand, sondern aktiv registrierte, getestete Module
(AirwindowsRegistry, je ein `*Tests.cpp`).

### 2.4 Klasse (c) — in Arbeit befindlich

**Keine.** Es gibt keine Datei, die unreferenziert wäre und deren
jüngste git-Timestamps auf eine laufende Baustelle deuteten.

**Vormerkung (heute KEIN Löschkandidat):**
`Source/UI/InputSendButton.{h,cpp}` ist laut Roadmap (CLAUDE.md §11,
I/O-Konsolidierung) zum Entfall vorgesehen, aktuell aber aktiv genutzt
(`Source/UI/ChannelAttributePanel.h`, `Source/UI/NodeComponent.h`).
Erst mit Umsetzung der I/O-Konsolidierung erneut prüfen.

---

## 3. Auskommentierte Blöcke

**Befund: 0 auskommentierte Codeblöcke > 10 Zeilen.**

Methodik: Scan über alle .h/.cpp beider Bäume auf (1) Läufe von > 10
aufeinanderfolgenden `//`-Zeilen mit mindestens 3 code-artigen Zeilen
(`;{}=()`-Gehalt) und (2) `/* */`-Blöcke > 10 Zeilen mit mindestens 4
Statement-artigen Zeilen. Die Blockkommentar-Suche lieferte 0 Treffer;
die vier `//`-Treffer sind nach Sichtung sämtlich Doku-Prosa mit
Code-Bezug, kein deaktivierter Code:

| Fundstelle | Inhalt |
|---|---|
| `Source/Core/Capture/CaptureService.cpp:41-51` | Begründung des Auto-Export-Sicherheitsnetzes in `prepare()` |
| `Source/UI/GridKeyboardComponent.h:168-178` | Hold/Drone-Semantik (Block M, User-Regeln 12.07.2026) |
| `Tests/Core/CaptureStressTests.cpp:13-24` | Testplan-Kopf des Stress-Suites |
| `Tests/DSP/Airwindows/AirwindowsTestHelpers.h:64-76` | DoD-Test-Doku (Denormal-Guard-Toleranz, Heap-Pflicht) |

---

## 4. Duplikations-Kandidaten

Vorab-Inventar zentraler Helfer: `Source/Util/` enthält nur
`SpscQueue.h`, `RtAllocationGuard.{h,cpp}`, `ScaleQuantizer.h`,
`CcNames.h` — es existiert keine zentrale Math-/dB-/Gesten-Util.
Bereits sauber zentralisiert (kein Handlungsbedarf):
`juce::SmoothedValue` für DSP-Gain-Smoothing, `Source/UI/
AnimatedValue.h` für UI-Slew, `CanvasGestureRecognizer` für
Pinch/Pan, `LiveFaderScale.h` (eigene Ableton-Domäne). Airwindows-
Ports als vendored Third-Party ausgeklammert.

### K1 — Meter-Ballistik (One-Pole-RMS + Peak-Decay + Warm-Start) · lohnt, Risiko mittel

`InputMeter` und `LevelMeter` teilen nahezu zeilengleiche Kernlogik:

- Koeffizienten `1 − exp(−1/(τ·sr))` / `exp(−blockSec/τ)`:
  `Source/Core/Capture/InputMeter.cpp:21` und `:48-49` ↔
  `Source/Core/Capture/LevelMeter.cpp:49-51` und `:72-74`; dieselbe
  Zeitkonstanten-Mathematik zusätzlich in `Source/Core/Metronome.cpp:11`.
- Sample-Schleife inkl. Warm-Start-Zweig (erster Block seedet mit
  Block-Mittel): `InputMeter.cpp:77-97` (Warm-Start `:90-91`) ↔
  `LevelMeter.cpp:131-152` (Warm-Start `:141-145`), Muster
  `blockPeak = jmax(...); meanSquare += rmsCoeff * (x*x − meanSquare);`.

Semantik-Unterschiede bleiben (InputMeter: Noise-Floor-Tracking;
LevelMeter: Peak-Hold + Clip-Latch). Risikoarm extrahierbar sind
zunächst die Koeffizienten-Helfer (`onePoleCoeff(τ,sr)`,
`blockDecay(τ,blockSec)`); der volle `MeterBallistics`-Baustein ist
Audio-Thread-Hotpath (noexcept/lock-free) und braucht Sorgfalt.

### K2 — dB → 0..1-Normierung · dupliziert TROTZ vorhandenem Helfer · lohnt, Risiko gering

Zentral existiert `LevelMeterBar::normFromLinear`
(`Source/UI/LevelMeterBar.cpp:35-42`). `GainFaderMeter` nutzt ihn für
den Balken (`Source/UI/GainFaderMeter.cpp:117-119`),
**re-implementiert dieselbe Formel aber inline** für die Skalen-Ticks
(`GainFaderMeter.cpp:176`). Weitere eigenständige
Pegel-Normierungen mit je eigener Floor-Konstante:
`Source/UI/CapturePanel.cpp:36-42` (`levelToSteps`, Floor −80 dB),
`Source/Core/Capture/CaptureService.cpp:170` (Floor −120 dB,
`CaptureService.h:464`), `Source/TouchLive/LiveSpectrumTap.cpp:249`.
Vorschlag: `normFromLinear(linear, floorDb)` als parametrierter freier
Helfer nach `Util/` (Floor als Argument statt fixer −60).

### K3 — Long-Press-/Tap-Geste · größter Code-Abbau, Risiko mittel

Dasselbe Zustandsmuster (mouseDown → `startTimer(longPressMs)`;
mouseDrag → Abbruch über `getDistanceFromDragStart() > Schwelle`;
mouseUp → Tap, wenn Long-Press nicht feuerte) ist mindestens 6-fach
handkopiert:

- `Source/UI/NodeColourDot.cpp:39-77` (Referenzform, 500 ms)
- `Source/UI/PushTiles.cpp:110-147` (`HoldIconTile`)
- `Source/UI/CapturePanel.cpp:46-59` + `.h:117` (`NameLabel`)
- `Source/UI/NodeComponent.cpp:1148-1264` + `.h:333-336`
- `Source/UI/MpeShapingView.cpp:83-112`
- `Source/UI/TouchLivePage/TouchLiveEq8Panel.cpp:401-414` (1000 ms)

Dazu zwei Timer-lose Varianten über `Time::getMillisecondCounter()`
(`Source/UI/LooperTrackStrip.cpp:648-654`, 600 ms;
`Source/UI/PortComponent.cpp:56,71`, Dwell 400 ms) und
Drag-Schwellen-Streuung in `Source/UI/Browser/BrowserListRow.cpp:133`,
`Source/UI/MasterDeviceSwitch.cpp:104`,
`Source/UI/TouchLivePage/TouchLiveGridView.cpp:421`.
Ein `LongPressGesture`-Helfer (Callbacks `onTap`/`onLongPress`,
Schwellen aus `UiSettings` — deckt sich mit Rule `node-editor`) würde
≥ 8 Kopien ablösen; Risiko: seitenspezifische Schwellen und
Multi-Finger-Kontexte, daher Umstellung seitenweise.

### K4 — MM:SS-Zeitformatierung · sofort, Risiko minimal

Zeichengleicher Ausdruck
`String(s/60) + ":" + String(s%60).paddedLeft('0',2)` an drei Stellen:
`Source/Core/Browser/BrowserModel.cpp:466-467`,
`Source/UI/LooperPage.cpp:83-84`,
`Source/Core/EngineEditor.cpp:414-415`.
Trivialer freier Helfer `formatMinutesSeconds(int)` nach `Util/`.

### K5 — ValueTree-Property-Casts · optional, hoher Umfang

Kein typisierter `getPropertyOr<T>`-Helfer; das Idiom
`(int)/(bool)/(float)(double) tree.getProperty(id, def)` streut über
~40 Dateien (z. B. `Source/TouchLive/LiveRemoteBridge.cpp:417,471,477,
546-548`, `Source/Core/EngineProcessor.cpp:754,791,879,886,910`,
`Source/Core/GridSessionStore.cpp:59-62`,
`Source/TouchLive/LiveTargetResolver.cpp:79-81`). Nutzen mittel,
Risiko gering, Diff sehr groß — eher Gelegenheits-Aufräumarbeit als
eigener Auftrag.

### Negativ-Befunde (bereits zentral — NICHT erneut extrahieren)

- **LCG-Zufall** (`1664525·state + 1013904223`): genau EINE
  Implementierung in `Source/Core/LinkSendTaps.cpp:278,280`
  (`convertToInt16Tpdf`); `Source/Modules/LinkAudioSendModule.cpp:313-317`
  ruft den zentralen Helfer auf. Übrige Zufalls-Nutzer verwenden
  `juce::Random` auf dem Message Thread (`StepSequencerModule.h:120`,
  `IStochastic.h`-Kontrakt).
- **DSP-/UI-Smoothing**: konsistent `juce::SmoothedValue` bzw.
  `AnimatedValue` — keine Streuung.
- **BPM/Hz/dB-String-Formatter**: nur Einzelvorkommen
  (`TouchLiveEq8Panel.cpp:37`), kein Extraktionsdruck.

---

## 5. TODO/FIXME-Inventar

18 Treffer, **alle** `TODO(design)` (bewusste, mit dem User zu
klärende Design-Entscheidungen — Konvention des Repos); **0** FIXME,
**0** HACK, kein nacktes technisches TODO:

| Datei:Zeile | Gegenstand |
|---|---|
| `Source/Core/CcControlModel.cpp:93` | Anzahl/Anordnung der Controls + CC-Zuweisung final vom User |
| `Source/Core/CcControlModel.h:92` | dito (Header-Doku) |
| `Source/Core/ChordMemory.h:34` | Persistenz der Chord-Slots (Preset/Settings) kommt später |
| `Source/Core/CurveEditInteraction.h:86` | Null-Lage/Mittelpunkt-Verhalten |
| `Source/Core/GridPanelSettings.h:94` | eigentliche Drag-Interaktion folgt separat |
| `Source/Core/PadGridLayout.h:36` | Default-Feinabstimmung mit dem User |
| `Source/UI/CcControlLayer.cpp:655` | MIDI-CC-Versand dockt hier an (CC-Nummern pro Control) |
| `Source/UI/CcPanel.cpp:17` | horizontales Padding im Mock nicht spezifiziert |
| `Source/UI/GridPage.cpp:64` | Modus-Verhalten (dort SPIELBAR) |
| `Source/UI/GridPage.cpp:1068` | Werte-Reset beim Neu-Bestücken akzeptiert |
| `Source/UI/GridPage.h:93` | Roadmap-Beschreibung/Modwheel-Toggle |
| `Source/UI/GridPage.h:115` | Werte-Reset akzeptiert (Header-Doku) |
| `Source/UI/GridPage.h:434` | echtes Laufzeit-Resize braucht CcControlLayer-Umbau |
| `Source/UI/GridSettingsView.h:37` | Drag-Select-Interaktion |
| `Source/UI/GridSettingsView.h:44` | Port/Kanal/CC-Empfang bleibt Block G |
| `Source/UI/MpeShapingView.h:130` | Hinweis auf EHEMALIGE TODO-Stelle (erledigt, Block A3) |
| `Source/UI/MpeShapingView.h:324` | dito |

(Die zwei MpeShapingView-Treffer sind historische Verweise auf eine
bereits erledigte Stelle, keine offenen Punkte — Zählung „offen": 16.)

**Alpha-v2-Referenzen (Phase 2.5):** 0 Treffer für
`Conduit_Alpha_v2`/`alpha v2`/alte Remote-URLs in aktiven Codepfaden
(Source/, Tests/, beide CMakeLists; case-insensitive). Historie
(Commits) laut CLAUDE.md-Kopf legitim und hier nicht gewertet.

---

## 6. Löschkandidaten-Shortlist (nur Vorschlag)

Löschung wäre je ein eigener Folgeauftrag nach Freigabe; Aufwand je
Kandidat: 2 Dateien löschen + 4 CMake-Einträge (je .cpp in
`CONDUIT_CPP_SOURCES` + ConduitTests-Liste, je .h in der
Header-Liste) + Kommentar-Nachpflege.

1. **`Source/UI/LinkAudioStatusBadge.{h,cpp}`** — seit 105eee5
   (01.07.2026) ohne Konsument; Ersatz (LinkAudioSendPanel +
   TransportBar-Integration) ist etabliert und feldnäher getestet.
   Nachpflege: „Muster"-Kommentare in `FxModulePanel.h:201` und
   `LinkAudioSendPanel.h:29` auf das Panel-Muster umformulieren.
   **Empfehlung: löschen.**
2. **`Source/UI/CaptureAllButton.{h,cpp}`** — seit 8467464
   (02.07.2026) ohne Konsument. VOR der Löschung eine
   User-Entscheidung einholen: die Mixer-Page-Roadmap („Capture-Buttons
   wandern dorthin") könnte eine Wiederverwendung meinen — allerdings
   wäre bei der Mixer-Page-Umsetzung ein Neuaufbau im dann gültigen
   Design wahrscheinlicher als die Reaktivierung einer 5 Wochen alten,
   ungetesteten Toolbar-Klasse. **Empfehlung: löschen, mit
   Roadmap-Vorbehalt.**

**Ausdrücklich KEINE Kandidaten:** `InputSendButton` (aktiv, Entfall
erst mit I/O-Konsolidierung), `TouchLiveEq8Panel` (Scheinbefund, lebt
über Bespoke-Registry), Airwindows-Ports (aktive Module),
`PORTING_NOTES.md` (Rule-referenziert), die 79 AUDIT-03-Header
(Pflege-Lücke, kein Dead Code).

---

*AUDIT-06 · read-only · keine Datei geändert oder gelöscht, kein
git-Kommando mit Schreibwirkung ausgeführt.*
