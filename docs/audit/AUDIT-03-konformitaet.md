# AUDIT-03 — Konformitäts-Audit CLAUDE.md v5.6 + Rules (Drift-Liste)

> Read-only-Abgleich Codebestand ↔ CLAUDE.md v5.6 + `.claude/rules/` ·
> Durchgeführt: 20.07.2026 · Branch: master (b1f998c) ·
> Vorgänger: AUDIT-01 (RT-Safety), AUDIT-02 (Threading)
>
> Pflichtlektüre erfüllt: CLAUDE.md vollständig; alle 12 Rules gelesen —
> `calibration`, `fx-chassis`, `grid`, `linkaudio`, `looper`, `midirig`,
> `node-editor`, `osc-remote`, `patch-engine`, `touchlive`, `transport`,
> `ui-design`. **Alle 12 verwenden `paths:`-Frontmatter, keine einzige
> `globs:`** (§1.1 erfüllt).

---

## 1. Zusammenfassung + Drift-Zähler

Der Codebestand ist in den harten, mechanisch prüfbaren Kernregeln
**vollständig konform**: kein APVTS (§2), kein gestauchter Text (§10.0),
C++20 strikt + Werror auf allen eigenen Sources aller vier Targets
(§13.1), RT-Guardrail-Defines korrekt und plattform-konditional (§13.2),
Sanitizer-Presets + CI vorhanden (§13.3), §4.4-Pflichtmethoden über die
Kategorien-Basisklassen sauber umgesetzt, UI hält nachweislich keine
Processor-/Modul-Pointer als Member (§5 — Zugriff läuft über
NodeUiRegistry bzw. ValueTree). Alle `.cpp`-Dateien beider Bäume sind in
den CMake-Targets registriert; der Airwindows-Testordner ist
deckungsgleich mit seiner CMake-Liste.

Die Drift konzentriert sich auf **Pflege-Lücken statt Regelbrüche**:
**79 Header** fehlen in den (rein IDE-relevanten) target_sources-
Header-Listen; die **Rules-`paths:` decken mehrere gewachsene
Subsystem-Dateien nicht mehr ab** (MIDI-Rig am stärksten); **45
Header-Dateien** enthalten Klassen ohne
`JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` (davon ~17 begründet:
abstrakte Interfaces/Kategorie-Basen); und **§13.5 kennt die real
existierenden Verzeichnisse `Source/DSP` und `Source/TouchLive` nicht**.

| Drift-Zähler | Anzahl |
|---|---|
| Harte Verstöße (Regel eindeutig gebrochen) | **0** |
| Drift-Befunde gesamt (D1–D8) | **8 Cluster** |
| — davon CMake/target_sources (D1) | 79 fehlende Header, 0 fehlende .cpp, 0 Waisen |
| — davon Struktur §13.5 (D2–D4) | 3 |
| — davon Makro-Abdeckung §2 (D5) | 45 Dateien (≈ 28 echte Kandidaten) |
| — davon UI-Framerate-Grenzfall (D6) | 1 |
| — davon Rules-`paths:`-Lücken (D7) | 6 Rules betroffen |
| — davon Doku-Inkonsistenz Rules (D8) | 1 |

---

## 2. Drift-Liste

| Regel | Datei:Zeile / Ort | Befund |
|---|---|---|
| **§2 APVTS** | — | ✓ 0 Treffer für `AudioProcessorValueTreeState`/`APVTS` in Source/ + Tests/ |
| **§2 Raw-Pointer** | `EngineProcessor.cpp:1164`, `Main.cpp:25`, `EngineEditor.cpp:113`, `LinkSendTaps.cpp:153`, `LinkClock.cpp:323/358`, `ChassisSchema.cpp:177`, `TouchLiveClient.cpp:654/798`, `BrowserPanel.cpp:229`, `TouchKeyboard.cpp:88` | ✓ alle `new`-Vorkommen idiomatisch mit sofortiger Ownership-Übergabe (JUCE `createEditor`/`setOwned`/ListBox-Row-Kontrakt, `unique_ptr`-Wrap privater Ctors, ref-counted `DynamicObject::Ptr`, `OwnedArray::add`) — keine rohen owning Pointer |
| **§2 LEAK_DETECTOR (D5)** | 45 Header (Heuristik: Klassen-Definitionen > Makro-Vorkommen) | Begründet ohne Makro: 12 `Source/Interfaces/I*.h` + `IRemoteTransport.h` (abstrakte Kontrakte ohne Member) und 5 Kategorie-Basisklassen (`AnalysisModule.h` etc., erben ConduitModule mit Makro). **Echte Kandidaten (~28)**, prominenteste mit Ressourcen-Membern: `MidiPortHub.h` (InputConnection hält Queues + Port-Handle; 4 Klassen/1 Makro), `LinkClock.h` (Sink/Source; 3/2), `CaptureSettings.h` (2/1), `OscSendService.h` (2/1), `ConduitMacroTargets.h` (3/2), `CapturePanel.h` (3/2), dazu je 1/0: CallbackTimingMonitor, SampleClock, CcControlModel, ChannelStripLayers, ChordMemory, GridPhysics, GridSessionStore, HardwareCcDatabase, LooperClipExporter, MidiInBindings, MidiTargetBrowserModel, MpeEncoder, NrpnAssembler, PadGridLayout, ParamModulation, RingTouchModel, TapTempo, CurvedSlider u. a. |
| **§5 UI-Bindung** | Source/UI (alle Header) | ✓ 0 Member/Referenzen vom Typ `EngineProcessor`/`ConduitModule`/`*Module` in UI-Headern; Modul-Zugriff über `NodeUiRegistry` (NodeCanvas/NodeComponent). Die Modul-Header-Includes in UI (`LooperPatchOutPanel.h:11` u. a.) dienen Statics/Enums/`id::` — regelkonform |
| **§10.0 Schrift (D-frei)** | `CcPanel.cpp:63`, `LinkAudioReceivePanel.cpp:181`, `MidiRigSettingsComponent.cpp:27` | ✓ alle `minimumHorizontalScale`-Vorkommen = 1.0; kein `drawFittedText` mit Scale < 1.0 |
| **§10.0 UI-Framerate (D6)** | `Source/Core/EngineEditor.cpp:532` (`startTimerHz (15)`) | Grenzfall: der zentrale Editor-Timer läuft mit 15 Hz — über der 10-Hz-Grenze, die Rule `ui-design` für Status-Polling erlaubt; Anzeige-Refreshes sollen über UiFramePacer laufen. Alle Source/UI-Timer sind konform: Status-Poller ≤ 10 Hz (LinkAudio-Panels/Badge/InputSendButton 10 Hz, HardwareTargetPicker 4 Hz), Rest sind Long-Press/Debounce/AutoSave — zulässige Nicht-Anzeige-Timer. `LiveSpectrumTap` 30 Hz ist Daten-Drain/FFT [MT], kein Anzeige-Refresh |
| **§13.1 (D-frei)** | `CMakeLists.txt:9–11, 563–567, 998–1002`; `Source/DSP/Airwindows/CMakeLists.txt:144, 167–171, 253–257` | ✓ `CMAKE_CXX_STANDARD 20` + `REQUIRED ON` + `EXTENSIONS OFF`; `/W4 /WX` bzw. `-Wall -Wextra -Werror` via `set_source_files_properties` auf **allen vier** Targets (Conduit, ConduitTests, ConduitAirwindows, ConduitAirwindowsTests) — directory-scoped, wirkt auch für die im Tests-Target erneut kompilierten Source-Dateien. JUCE via FetchContent 8.0.4 ✓, Link via FetchContent ✓, Catch2 v3 ✓ |
| **§13.2 (D-frei)** | `CMakeLists.txt:572–581, 1004–1010`; AW: `:159–162` | ✓ `JUCE_MODAL_LOOPS_PERMITTED=0`, `JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0` auf Conduit + ConduitTests; Plattform-Defines strikt konditional (`APPLE`/`WIN32` + `JUCE_ASIO_SDK_PATH`-Gate, `:584–595`). Randnotiz: ConduitAirwindows trägt CURL/WEB_BROWSER, aber kein MODAL_LOOPS — unkritisch, die Lib linkt nur `juce_audio_basics` (kein Message-Loop-Code) |
| **§13.3/13.4 (D-frei)** | `CMakePresets.json`, `.github/workflows/ci.yml` | ✓ Presets `asan`/`tsan`/`asan-linux`/`windows-vs` vorhanden; TSan Clang-gated (`CMakeLists.txt:25–31`); CI-Workflow existiert |
| **§4.4 (D-frei)** | Stichprobe 5 Module: Attenuator, Scope, Lfo, StepSequencer, LooperPatchOut (+ LinkAudioReceive) | ✓ `getModuleId`/`getModuleDisplayName`/`getStateVersion` je Modul implementiert; `getType()` über die Kategorie-Basisklassen (`UtilityModule.h:16` etc.); `createState()` Basis-Implementierung + Overrides wo nötig (LooperPatchOut/PatchIn/LinkAudioSend/Receive) — lazy, vor addNode |
| **§6 ValueTree** | Querverweis | ✓ AUDIT-01 (kein ValueTree-Zugriff aus Audio-Pfaden) und AUDIT-02 (Mutationen MT-seitig, OSC-Pfad-2 via AsyncUpdater) — hier nicht erneut tief geprüft |
| **§13.5 (D2)** | `Source/DSP/`, `Source/TouchLive/` | Verzeichnisse existieren und tragen produktiven Code, sind aber in §13.5 (`Source/{Core,Modules,Interfaces,UI,Util}`) **nicht dokumentiert** → Abschnitt 5 |
| **§13.5 (D3)** | `Tests/` | Spiegelung unvollständig: kein `Tests/Interfaces` (header-only Kontrakte — vertretbar), kein `Tests/Modules` — Modul-Tests liegen in `Tests/Core` (AirwindowsModuleTests, ProcessorChassisTests, LooperPatch\*ModuleTests, ScopeModuleTests …) |
| **§13.5 (D4)** | `Source/Modules/{Analysis,Generator,IO,NetworkIO,Utility}Module.h`, `Source/Core/Main.cpp` | Wortlaut „je Modul ein .h/.cpp-Paar": 5 Kategorie-Basisklassen sind header-only (Einzeiler-Basen — vertretbar); Main.cpp ohne Header (üblich). 46 header-only Dateien gesamt, überwiegend legitim (Interfaces, Math/POD, Templates wie SpscQueue) |

---

## 3. target_sources-Abgleich (D1)

Basis: 824 Dateien (.h/.cpp) auf Platte unter Source/ + Tests/ vs. beide
CMakeLists (Airwindows-Tests über `${CONDUIT_AIRWINDOWS_TESTS_DIR}`
aufgelöst).

**In CMake, aber NICHT auf Platte (Waisen): 0.**

**Auf Platte, aber NICHT in target_sources:**

- **`.cpp`: 0 fehlend** — alle Übersetzungseinheiten beider Bäume sind
  registriert; `Tests/DSP/Airwindows/*.cpp` (58) deckungsgleich mit der
  Variablen-Liste im AW-CMakeLists (diff leer).
- **`.h`: 79 fehlend** (nur IDE-Sichtbarkeit, kein Build-Einfluss).
  Auffällige Cluster:
  - **komplette Looper-Familie:** `Source/Core/Looper/*.h` (9 Dateien:
    LooperBank, LooperClip, BarSampleAnchors, LooperTrashCan, …),
    `Source/Core/LooperSettings.h`, alle 17 `Source/UI/Looper*.h`
  - **komplette MIDI-Rig-Familie:** `MidiPortHub.h`, `MidiInBindings.h`,
    `MidiRigSettings.h`, `NrpnAssembler.h`, `RelativeEncoding.h`,
    `PickupLedRouter.h`, `PositionFeedbackRouter.h`,
    `ControllerProfile(-Library).h`, `HardwarePreset*(.h)`,
    `Sysex/DsiSysex.h`, `ChannelStripLayers.h`,
    `MidiControllerEvent.h`, `MidiSysexEvent.h`, `MidiDeviceProfile.h`,
    `MidiProfileLibrary.h`, `MidiTargetBrowserModel.h`,
    `MidiRigSettingsComponent.h`
  - **einzelne jüngere Header:** `Interfaces/ILooperAudioClient.h`,
    `Modules/LinkAudioReceiveModule.h`,
    `Modules/LooperPatchInModule.h`, `Modules/LooperPatchOutModule.h`,
    `Core/LinkReceiveStream.h`, `Core/CallbackTimingMonitor.h`,
    `Core/Capture/LevelMeter.h`, `UI/UiFramePacer.h`,
    `Core/SignalFlowColours.h`, `Core/LaunchQuantization.h` u. a.
  - **Test-Helfer** (üblich, kein Befund): `Tests/Core/Browser/TestDispatcher.h`,
    `FakeMidiTarget.h`, `FakeVoiceSink.h`, `TestSettingsFolder.h`
  - Muster: die Header-Listen wurden seit etwa dem Looper-/MIDI-Rig-
    Ausbau (Juli 2026) nicht mehr nachgeführt — ältere Subsysteme
    (Capture, Browser, TouchLive, Airwindows) sind vollständig.

---

## 4. Rules-Befunde (D7/D8)

**Frontmatter: alle 12 Rules `paths:`, keine `globs:` — §1.1 ✓.**

**Tote Pfad-Muster:**

| Rule | Muster | Befund |
|---|---|---|
| `calibration` | `Source/**/*Calibrat*`, `Source/**/*CVTuner*`, `Source/**/*HardwareIO*`, `Tests/**/*Calibrat*` | **4 tote Muster** — 0 Dateien auf Platte (CV-Subsystem ist v2.0-Roadmap, noch nicht implementiert). Rule triggert real nur über `ChannelNames.h`/`EngineProcessor.*`. Vorausgreifend angelegt — kein Fehler, aber als Zustand dokumentierenswert |

**Abdeckungslücken (Subsystem-Dateien, die die eigene Rule nicht
triggern — Trigger dann höchstens über `ui-design` bzw. gar nicht):**

| Rule | Nicht gedeckte Bestandsdateien |
|---|---|
| `midirig` | `NrpnAssembler.h`, `RelativeEncoding.h`, `ChannelStripLayers.*`, `PickupLedRouter.*`, `PositionFeedbackRouter.*`, `ControllerProfile.*`, `ControllerProfileLibrary.*`, `HardwarePresetLibrary.*`, `HardwarePresetScanner.*`, `Source/Core/Sysex/**`, `Source/UI/MidiRigSettingsComponent.*`, sämtliche MIDI-Rig-Tests (Rule listet als einzige Subsystem-Rule keine Test-Pfade) — die Rule-Texte referenzieren diese Dateien selbst (PickupLedRouter, RelativeEncoding.h, DsiSysex), ihre `paths:` laden sie aber nicht |
| `grid` | `GridPhysics.*`, `GridSessionStore.*`, `GridSensitivity.h`, `ChordMemory.*`, `CcControlModel.*`, `Source/UI/CcControlLayer.*`, `Source/UI/ChordMemoryStrip.*`; Tests: `GridPhysicsTests`, `GridSessionStoreTests`, `GridSensitivityTests`, `ChordMemoryTests`, `CcControlModelTests` |
| `looper` | `Source/Core/LooperSettings.*` (liegt außerhalb `Source/Core/Looper/`; der zugehörige Test `LooperSettingsTests` matcht dagegen via `Tests/Core/Looper*`) |
| `transport` | `Source/Core/TapTempo.h` (Tap-Tempo gehört fachlich zur TransportBar) |
| `node-editor` | `Source/UI/NodeAttributePanel.*`, `NodeColourDot.*`, `PortComponent.*`; `Tests/UI/NodeCanvasTests.cpp` |
| (keine Rule) | **`Source/Core/EngineEditor.*`** — der Editor (UI-Wurzel, LookAndFeel-Setup, 15-Hz-Timer) liegt in Core und wird von keiner einzigen Rule getriggert; `ui-design` greift nur unter `Source/UI/**` |

**Konsistenz (D8):** `node-editor` ist die einzige Subsystem-Rule, deren
`paths:` das eigene Dossier (`docs/NodeEditor.md`) **nicht** enthält —
alle anderen Rules laden bei Dossier-Bearbeitung mit.

**Fehlende Rules zu Subsystemen mit Dossier:** Alle 12 Dossiers unter
`docs/` haben eine zugehörige Rule oder sind Querschnitt (Build.md,
DataModel.md → bewusst unconditional in CLAUDE.md §6/§13; OscSend.md +
M4LAnnounce.md → `osc-remote`). Kein fehlendes Rule-Pendant.

---

## 5. Lücken der CLAUDE.md selbst (Code lebt Regeln, die nirgends stehen)

1. **`Source/DSP/` und `Source/TouchLive/` fehlen in §13.5.** Beide
   existieren seit Wochen (Airwindows-Lib mit eigenem CMake-Target;
   TouchLive-Remote als eigene Schicht mit eigenem Tests-Spiegel) und
   werden von Rules referenziert (`fx-chassis`: `Source/DSP/**`;
   `touchlive`: `Source/TouchLive/**`) — die Projektstruktur-Regel
   kennt sie nicht.
2. **Das Static-Library-Muster ConduitAirwindows** (eigenes CMake-Target
   via `add_subdirectory`, eigenes Test-Executable, Include-Root-
   Konvention `DSP/Airwindows/...`) ist nirgends dokumentiert — §13.3
   beschreibt nur die zwei Haupt-Targets.
3. **Werror-Scope-Muster:** „`/WX` nur auf eigene Sources via
   `set_source_files_properties`, JUCE-Sources bleiben frei" ist eine
   gelebte, in CMake-Kommentaren begründete Konvention — §13.1 sagt
   pauschal „Warnungen als Fehler" ohne diese wichtige Einschränkung.
4. **NodeUiRegistry als sanktionierter UI→Modul-Weg:** §5 verbietet
   Processor-Pointer in der UI; dass der legitime Ersatzweg die
   NodeUiRegistry ist (mit VBlank-Teardown), steht nur im Rule-/
   Header-Text, nicht in der CLAUDE.md-Kernregel.
5. **Header-Registrierung in target_sources** (IDE-Sichtbarkeit) wird
   faktisch gepflegt (700+ Einträge), ist aber nirgends als Pflicht
   dokumentiert — entsprechend ist sie stillschweigend verwaist (D1).
   Entweder Pflicht dokumentieren oder Praxis aufgeben.
6. **Kategorie-Basisklassen sind bewusst header-only** — §13.5-Wortlaut
   („je Modul ein .h/.cpp-Paar") kennt diese Ausnahme nicht.
7. Bereits in AUDIT-01/02 erfasst, hier nur gezählt: §3.1-Wortlaut vs.
   `IStochastic.h`-RNG-Kontrakt; §6.1-Undo-Guard enger implementiert
   als dokumentiert; DataModel-6.1-Skizze veraltet.

---

## 6. Empfohlene Folgeaufträge

**Code-Fix (kein Doku-Hub nötig):**

1. **(mittel)** target_sources-Header-Listen nachführen (79 Einträge,
   Abschnitt 3) — rein mechanisch, ein Commit; alternativ die
   Header-Listen komplett streichen (siehe Doku-Fix 3).
2. **(mittel)** LEAK_DETECTOR-Nachrüstung priorisiert auf die ~10
   Ressourcen-haltenden Klassen (MidiPortHub::InputConnection,
   LinkClock::Sink/Source, CaptureSettings, OscSendService,
   LooperClipExporter, HardwareCcDatabase, MidiInBindings, GridSessionStore,
   ChannelStripLayers); Interfaces/Kategorie-Basen bewusst auslassen.
3. **(niedrig)** EngineEditor-Timer: 15 Hz → UiFramePacer-Tick oder
   10 Hz (plus Umzug der Datei-Zuständigkeit, siehe Doku-Fix 2).

**Doku-/Rules-Fix (CLAUDE.md-Versionshub v5.6 → v5.7; kein ADR-Bedarf,
da keine Architektur-Entscheidung — reine Nachführung):**

1. **(hoch)** Rules-`paths:` nachführen: midirig (+11 Muster inkl.
   Tests), grid (+5), looper (+`Source/Core/LooperSettings.*`),
   transport (+TapTempo.h), node-editor (+docs/NodeEditor.md, +3 UI-
   Dateien), NEU: `Source/Core/EngineEditor.*` einer Rule zuordnen
   (ui-design-paths erweitern oder patch-engine). Kein ADR — ADR 005
   deckt den Mechanismus, das ist Pflege.
2. **(mittel)** §13.5 aktualisieren: `Source/{Core,DSP,Interfaces,
   Modules,TouchLive,UI,Util}`, Tests-Spiegel-Realität (Modul-Tests in
   Tests/Core), header-only-Ausnahmen (Interfaces, Kategorie-Basen,
   Math/Templates). Versionshub v5.7.
3. **(mittel)** §13.1/§13.3 präzisieren: Werror-Scope-Muster,
   ConduitAirwindows-Target + Include-Konvention; Entscheidung
   Header-Registrierungs-Pflicht (→ danach Code-Fix 1 ausführen oder
   Listen löschen).
4. **(niedrig)** calibration-Rule: Kommentar „Muster vorausgreifend,
   Subsystem v2.0" ins Rule-File, damit künftige Audits die toten
   Pfade nicht erneut untersuchen.
5. **(niedrig, gebündelt mit AUDIT-01/02-Folgeaufträgen)** §3.1↔
   IStochastic harmonisieren, §6.1-Undo-Guard-Wortlaut, DataModel-
   Skizze — EIN gemeinsamer Doku-Commit v5.7 bietet sich an.
