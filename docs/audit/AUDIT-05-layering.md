# AUDIT-05 — Abhängigkeitsgraph & Layering

> Read-only-Audit, 20.07.2026 · Referenz: CLAUDE.md v5.6 (Version-Check bestanden)
> Datenbasis: 641 Dateien unter `Source/`, 1241 projektinterne Include-Kanten
> (nur `#include "…"`; JUCE/System/BinaryData ignoriert — 4 nicht auflösbare
> Includes waren sämtlich `BinaryData.h`, extern generiert).

---

## 1. Zusammenfassung

- **Keine Include-Zyklen** — weder direkte 2er-Zyklen noch größere
  Strongly-Connected-Components auf Datei-Ebene. Der Graph ist ein sauberer DAG.
- **Das Soll-Layering des Auftrags traf nicht auf den Ist-Zustand zu** (79
  Kanten dagegen) — gemäß STOPP-Bedingung wurde vor Phase 2 angehalten. Vom
  User bestätigtes Soll (20.07.2026): **Core ist die App-Kompositionsschicht**
  (`EngineProcessor`, `EngineEditor`, `GraphManager`, `Main` dürfen Modules/UI/
  TouchLive instanziieren); als Verstoß gelten UI→Modules (§5) sowie
  Core→Modules/UI aus Nicht-Kompositions-Dateien. `Source/DSP` und
  `Source/TouchLive` (in §13.5 nicht genannt) werden als Schichten ergänzt:
  DSP unter Modules (nur Modules→DSP), TouchLive neben Core.
- **Verstoßbild:** 30 direkte UI→Modules-Kanten, dazu eine **transitive
  §5-Kette**: 10 UI-*Header* inkludieren `Core/GraphManager.h`, der selbst
  `Modules/ConduitModule.h` voll inkludiert — obwohl er ihn nur als
  Pointer/Referenz nutzt (Forward-Declaration möglich). 12 Core→Modules- und
  1 Core→UI-Kante stammen aus Nicht-Kompositions-Dateien.
- **Positiv:** `Source/DSP` hat **null** ausgehende Kanten (perfekt gekapselte
  Engine-Schicht), Util/Interfaces sind sauber, `Modules→Core` beschränkt sich
  auf Tap-/Clock-/Capture-Dienste.
- Kein struktureller Blocker für die Roadmap, aber drei Kantenstränge
  verteuern Mixer-Page, I/O-Konsolidierung und CLAP-Hosting (Abschnitt 6).

---

## 2. Aggregierte Kantenliste (Verzeichnis-Ebene) + Soll/Ist-Abgleich

### 2.1 Bestätigtes Soll-Layering (Annahme, vom User bestätigt 20.07.2026)

Begründung aus CLAUDE.md: §13.5 nennt die Verzeichnisse ohne Richtungen; §5
liefert die einzige harte Include-Regel (UI bindet nur an ValueTree-Subtree,
nie an Processor); §4 macht Modules von den Basisklassen/Interfaces abhängig.
Die Kompositionsrolle von Core ist aus der MD nicht ableitbar, entspricht aber
dem gewachsenen Ist-Modell und wurde als Soll bestätigt:

```
Util        → (nichts)
Interfaces  → Util
DSP         → (nichts)                        [neu: reine Engine-Schicht]
Core        → Interfaces, Util                [Nicht-Kompositions-Dateien]
Core-Komposition (EngineProcessor, EngineEditor, GraphManager, Main)
            → zusätzlich Modules, UI, TouchLive   [legitim]
TouchLive   → Core, Util                      [neu: Subsystem neben Core]
Modules     → Core, Interfaces, Util, DSP
UI          → Util, TouchLive, Core (ValueTree-/Settings-/Service-Header;
              KEINE Processor-/Modul-Header, §5)
```

### 2.2 Ist-Kanten und Abgleich

| Kante | Anzahl | Soll-Abgleich |
|---|---:|---|
| Util → Util | 1 | ✅ konform (intra) |
| Interfaces → Interfaces | 1 | ✅ konform (intra) |
| DSP → DSP | 174 | ✅ konform; **0 ausgehende Kanten** |
| Core → Core | 219 | ✅ intra |
| Core → Interfaces / Util | 21 / 12 | ✅ konform |
| Core → Modules | 26 | ⚠️ 14 aus Komposition ✅, **12 Verstoß** (4.2) |
| Core → UI | 23 | ⚠️ 22 aus Komposition ✅, **1 Verstoß** (4.3) |
| Core → TouchLive | 6 | ✅ alle aus `EngineProcessor.h` (Komposition) |
| TouchLive → TouchLive / Core / Util | 18 / 7 / 1 | ✅ konform |
| Modules → Modules / DSP | 216 / 59 | ✅ konform |
| Modules → Core / Interfaces / Util | 10 / 16 / 2 | ✅ konform (nur Tap/Clock/Capture-Dienste) |
| UI → UI | 253 | ✅ intra |
| UI → Core | 109 | ⚠️ überwiegend konform; **10 GraphManager.h-Includes in UI-Headern = transitive §5-Kette** (4.1b) |
| UI → Modules | 30 | ❌ **Verstoß §5** (4.1a) |
| UI → TouchLive / Util / Interfaces | 30 / 6 / 1 | ✅ konform |

---

## 3. Zyklen

**Keine.** Tarjan-SCC über alle 641 Dateien liefert ausschließlich
Einzelknoten; auch keine direkten A↔B-Paare. Auf Verzeichnis-Ebene existiert
formal der Ring Core→UI→Core bzw. Core→Modules→Core — er ist auf Datei-Ebene
azyklisch und durch die Kompositionsrolle von Core erklärt (EngineEditor
inkludiert Pages, Pages inkludieren Core-Dienste, nie den Editor zurück).

---

## 4. Verstoß-Tabelle (Datei-Ebene)

### 4.1a UI → Modules (30 Kanten, §5)

Drei Untergruppen nach Schwere:

**(i) Echte Processor-Zugriffe** — UI holt sich Modul-Pointer aus dem
GraphManager (Muster: `dynamic_cast<XModule*>(graphManager.getModuleFor(uuid))`,
z. B. `ScopeDisplay.cpp:27`). Funktional für Realtime-Datenpfade (Scope-Queue,
Meter), aber genau das Zombie-UI-Muster, das §5 verhindern soll:

| Datei:Zeile | inkludiert |
|---|---|
| `Source/UI/ScopeDisplay.cpp:3` | `Modules/ScopeModule.h` |
| `Source/UI/StepGridDisplay.cpp:3` | `Modules/StepSequencerModule.h` |
| `Source/UI/NodeComponent.cpp:8` | `Modules/ScopeModule.h` |
| `Source/UI/NodeComponent.cpp:9` | `Modules/StepSequencerModule.h` |
| `Source/UI/GainFaderMeter.cpp:5` | `Modules/ProcessorModule.h` |
| `Source/UI/FxModulePanel.cpp:4` | `Modules/ProcessorModule.h` |
| `Source/UI/LinkAudioReceivePanel.h:9` | `Modules/LinkAudioReceiveModule.h` |
| `Source/UI/LinkAudioSendPanel.h:9` | `Modules/LinkAudioSendModule.h` |
| `Source/UI/LinkAudioSendPanel.cpp:3` | `Modules/ConduitModule.h` |
| `Source/UI/LinkAudioStatusBadge.h:6` | `Modules/LinkAudioSendModule.h` |
| `Source/UI/LinkSendCreateDialog.h:8` | `Modules/LinkAudioSendModule.h` |
| `Source/UI/LooperPatchInPanel.h:9` | `Modules/LooperPatchInModule.h` |
| `Source/UI/LooperPatchOutPanel.h:11` | `Modules/LooperPatchOutModule.h` |
| `Source/UI/NodeCanvas.cpp:7` | `Modules/AttenuatorModule.h` |
| `Source/UI/NodeCanvas.cpp:8` | `Modules/LooperPatchOutModule.h` |
| `Source/UI/NodeComponent.cpp:6` | `Modules/LinkAudioSendModule.h` |
| `Source/UI/NodeComponent.cpp:7` | `Modules/LooperPatchInModule.h` |

**(ii) ChassisSchema-Nutzer** — inkludieren `Modules/ChassisSchema.h`, ein
laut Eigen-Doku „pures, statisches" Schema (Rollen-Strings, Kanal-Layout),
das seinerseits `ConduitModule.h` zieht. Kein Processor-Zugriff, aber die
Datei liegt im falschen Layer:

| Datei:Zeile | inkludiert |
|---|---|
| `Source/UI/ConduitTargetPicker.cpp:3` | `Modules/ChassisSchema.h` |
| `Source/UI/CurveEditor.h:7` | `Modules/ChassisSchema.h` |
| `Source/UI/CurvedSlider.h:7` | `Modules/ChassisSchema.h` |
| `Source/UI/FxModulePanel.cpp:3` | `Modules/ChassisSchema.h` |
| `Source/UI/FxModulePanel.h:10` | `Modules/ChassisSchema.h` |

**(iii) ConduitModule-Basisklassen-Includes** — meist für Typen/Enums
(`ModuleType`, IDs); teils laut Heuristik gar nicht im Header verwendet:

| Datei:Zeile | inkludiert |
|---|---|
| `Source/UI/ConduitTargetPicker.h:9` | `Modules/ConduitModule.h` |
| `Source/UI/GridPage.cpp:9` | `Modules/ConduitModule.h` |
| `Source/UI/GridSettingsView.cpp:6` | `Modules/ConduitModule.h` |
| `Source/UI/NodeComponent.cpp:5` | `Modules/ConduitModule.h` |
| `Source/UI/PageOverviewComponent.cpp:3` | `Modules/ConduitModule.h` |
| `Source/UI/ParameterPanel.h:8` | `Modules/ConduitModule.h` |
| `Source/UI/SequencerControlPanel.cpp:3` | `Modules/ConduitModule.h` |
| `Source/UI/TransportBar.cpp:5` | `Modules/ConduitModule.h` |

### 4.1b Transitive §5-Kette: UI-Header → `Core/GraphManager.h` → `Modules/ConduitModule.h`

`Core/GraphManager.h:12` inkludiert `ConduitModule.h` voll, nutzt den Typ aber
nur als Pointer/Referenz (Forward-Declaration genügt, siehe 5.2). Damit ziehen
diese 10 UI-Header die komplette Modul-Basisklasse transitiv:

`UI/FxModulePanel.h:8`, `UI/GainFaderMeter.h:5`, `UI/LinkAudioReceivePanel.h:8`,
`UI/LinkAudioSendPanel.h:8`, `UI/LinkAudioStatusBadge.h:5`,
`UI/LooperPatchInPanel.h:8`, `UI/NodeCanvas.h:13`, `UI/NodeComponent.h:11`,
`UI/ScopeDisplay.h:7`, `UI/StepGridDisplay.h:5`

(`GraphManager.h:13` inkludiert zusätzlich `Modules/LooperPatchOutModule.h` —
gleiche Kette.)

### 4.2 Core (Nicht-Komposition) → Modules (12 Kanten, 11 Dateien)

| Datei:Zeile | inkludiert | Anmerkung |
|---|---|---|
| `Source/Core/Browser/BrowserModel.h:13` | `Modules/ModuleFactory.h` | nur Ptr/Ref → Forward-Decl möglich |
| `Source/Core/ConduitMacroTargets.cpp:3` | `Modules/ConduitModule.h` | |
| `Source/Core/Looper/LooperTrashCan.h:7` | `Modules/LooperPatchOutModule.h` | |
| `Source/Core/ModuleUiDefaults.cpp:3` | `Modules/ChassisSchema.h` | entfällt bei Schema-Umzug (7.2) |
| `Source/Core/OscAddress.h:5` | `Modules/ConduitModule.h` | Heuristik: Typ im Header ungenutzt |
| `Source/Core/OscController.cpp:4` | `Modules/ConduitModule.h` | |
| `Source/Core/OscSendService.cpp:5` | `Modules/ConduitModule.h` | |
| `Source/Core/PageManager.h:5` | `Modules/ConduitModule.h` | Heuristik: Typ im Header ungenutzt |
| `Source/Core/RemoteModuleBinder.cpp:4/5` | `ConduitModule.h`, `ModuleFactory.h` | |
| `Source/Core/SignalFlowColours.cpp:7` | `Modules/ConduitModule.h` | |
| `Source/Core/SourceNameResolver.cpp:7` | `Modules/ConduitModule.h` | |

Gemeinsame Ursache: Diese Dienste brauchen `moduleId`/`displayName`/Farb- und
Typinformationen — d. h. ein schmales Metadaten-Interface, nicht die
AudioProcessor-Basisklasse.

### 4.3 Core (Nicht-Komposition) → UI (1 Kante)

| Datei:Zeile | inkludiert |
|---|---|
| `Source/Core/GridPanelSettings.cpp:3` | `UI/PushLookAndFeel.h` |

(Vermutlich für Push-Farbkonstanten — Kandidat: Farbpalette nach
`Core`/`Util` extrahieren oder Settings-Farbauflösung in die UI verlagern.)

### 4.4 Legitim per bestätigtem Soll (keine Verstöße, zur Transparenz)

- Core-Komposition → Modules (14): `EngineProcessor.h:49–50`,
  `EngineEditor.cpp:7–11`, `GraphManager.cpp:14–19`, `GraphManager.h:12–13`.
- Core-Komposition → UI (22): `EngineEditor.h/.cpp`, `Main.cpp:6–7`.
- Core-Komposition → TouchLive (6): `EngineProcessor.h:25–30`.
- Modules → Core (10): ausschließlich `CaptureService`, `LinkClock`,
  `LinkReceiveStream`, `LinkSendTaps`, `LevelMeter`, `LooperBank` —
  Dienst-Header, kein Rückgriff auf Komposition.
- TouchLive → Core (7): `MacroBindings`, `ControllerProfileLibrary`,
  `MidiPortHub`, `MidiRigSettings`, `PositionFeedbackRouter`, `LinkClock`.
- UI → TouchLive (30): Client/Model/Taps — konsistent mit docs/TouchLive.md.

---

## 5. Hotspots + Forward-Declaration-Kandidaten

### 5.1 Top-10 meist-inkludierte projektinterne Header

| # | Header | Includer | Bewertung |
|---:|---|---:|---|
| 1 | `DSP/Airwindows/AirwindowsPlugin.h` | 60 | Airwindows-Familie (54 Module) — strukturell erwartbar |
| 2 | `UI/PushLookAndFeel.h` | 59 | Design-System; prüfen, ob Farb-/Metrik-Konstanten in leichten Sub-Header wandern können |
| 3 | `Modules/AirwindowsProcessorModule.h` | 58 | zieht ProcessorModule+Chassis in jede Airwindows-TU |
| 4 | `Modules/ConduitModule.h` | 28 | **Schlüssel-Hotspot**: hängt via GraphManager.h transitiv in der halben UI (4.1b) |
| 5 | `UI/PushTiles.h` | 21 | Design-System, ok |
| 6 | `Core/GraphManager.h` | 15 | 10 davon UI-Header → 5.2 |
| 7 | `TouchLive/LiveSetModel.h` | 14 | ok |
| 8 | `TouchLive/TouchLiveClient.h` | 12 | in 8 UI-Headern nur Ptr/Ref → Forward-Decl |
| 9 | `Core/Capture/CaptureService.h` | 11 | ok |
| 10 | `Core/LinkClock.h` / `Util/SpscQueue.h` | je 10 | ok (SpscQueue = gewollter Standard-Baustein) |

### 5.2 Forward-Declaration-Kandidaten (Heuristik: Typ nur als Pointer/Referenz bzw. im Header ungenutzt; 71 von 283 cross-dir Header→Header-Kanten)

**Höchster Hebel (kappt Verstoß- oder Hotspot-Ketten):**

| Header | inkludiert | Befund |
|---|---|---|
| `Core/GraphManager.h` | `Modules/ConduitModule.h` | nur Ptr/Ref → **kappt die transitive §5-Kette für 10 UI-Header** |
| `Core/EngineProcessor.h` | `Modules/ConduitModule.h`, `Interfaces/IClockSource.h` | im Header ungenutzt (Heuristik) |
| `Core/Browser/BrowserModel.h` | `Modules/ModuleFactory.h` | nur Ptr/Ref |
| `Core/OscAddress.h`, `Core/PageManager.h` | `Modules/ConduitModule.h` | im Header ungenutzt (Heuristik) |
| `UI/GridPage.h` | 7 Core-/TouchLive-Header (`ControllerProfileLibrary`, `MidiPortHub`, `MidiProfileLibrary`, `HardwarePreset*`, `LinkClock`, `TouchLiveClient`) | nur Ptr/Ref |
| `UI/MacroPanel.h` | 6 Header (u. a. `HardwareCcDatabase`, `TouchLiveClient`, `IMidiOutputTarget`) | nur Ptr/Ref |
| `UI/TouchLivePage/*.h` (5 Views) | `TouchLiveClient`, `LiveSpectrumTap`, `TouchLiveMeterBus`, `LiveSetModel` | nur Ptr/Ref |

**Vermutlich tote Includes** (Heuristik „Typ kommt im Header nicht vor" —
vor Entfernung kompilierend verifizieren, Makro-/Vererbungsnutzung möglich):
`Interfaces/IClockSource.h` in 6 Core-Headern (`LinkReceiveStream`,
`Looper/BarSampleAnchors`, `Looper/LooperBank`, `Looper/LooperWaveformTap`,
`Metronome`, `GraphManager`), `Util/ScaleQuantizer.h` in 4 UI-Headern,
`DSP/…/{KBeyond,KCathedral5,KWoodRoom}.h` in ihren Modul-Headern.

Vollständige Kandidatenliste: 71 Einträge, im Audit-Lauf erhoben
(Skript-Heuristik; Rohliste auf Anfrage reproduzierbar).

---

## 6. Roadmap-Relevanz (§11)

**Mixer-Page (∥∥, Channel-Strips, Capture-Buttons):** Channel-Strips brauchen
Meter- und Gain-Daten pro Modul. Das heutige UI-Muster dafür
(`GainFaderMeter.cpp`/`FxModulePanel.cpp` → `ProcessorModule.h`,
Modul-Pointer via GraphManager) würde sich mit jedem Strip multiplizieren.
Core besitzt bereits die richtigen Bausteine (`Core/Capture/LevelMeter.h`,
`Core/LinkSendTaps.h`, `looperOutLevels`-Muster der Patch-OUT-Kachel) — vor
dem Mixer-Bau sollte der Meter-/Level-Datenpfad einheitlich als Core-Tap
(SPSC/atomic, UI liest nur Tap + ValueTree) definiert sein, sonst zementiert
die Mixer-Page 4.1a-(i) als Standard.

**I/O-Konsolidierung (audio_input/audio_output + „+"-Kanäle):** Die heutigen
vier Link-Audio-UI-Dateien (`LinkAudioSendPanel`, `LinkAudioReceivePanel`,
`LinkAudioStatusBadge`, `LinkSendCreateDialog`) binden direkt an Modul-Header.
Bei der Konsolidierung werden diese Panels ohnehin umgebaut — das ist der
natürliche Zeitpunkt, die Panel↔Modul-Kopplung auf reine
ValueTree-Subtree-Bindung (§5) umzustellen, statt die Kanten ins neue
konsolidierte I/O-Panel zu übertragen. Gleiches gilt für
`LooperPatchIn/OutPanel` (ADR 012/013).

**CLAP-Hosting (PluginModule wraps AudioPluginInstance):** Ein generisches
Plugin-UI muss aus Schema-Daten rendern, ohne den konkreten Modul-Typ zu
kennen. Dem steht entgegen: (a) `ChassisSchema.h` liegt in Modules und zieht
`ConduitModule.h` — als Core-Schema wäre es für generische UIs frei
inkludierbar; (b) das `dynamic_cast`-Muster (4.1a-i) funktioniert für
gehostete Plugins prinzipiell nicht — der Datenpfad muss typunabhängig sein.
Compile-Zeit-seitig zeigt der Airwindows-Hotspot (58 Includer auf
`AirwindowsProcessorModule.h`), wie teuer Modul-Familien-Header werden;
für CLAP gilt: Wrapper-Header schlank halten.

Kein Befund blockiert die Roadmap hart — der Graph ist zyklenfrei und die
Schichten sind im Kern tragfähig. Die drei Stränge oben sind
„Jetzt-billig-später-teuer"-Kanten.

---

## 7. Empfohlene Folgeaufträge

1. **GraphManager.h entkoppeln** (klein, hoher Hebel): `ConduitModule`/
   `LooperPatchOutModule` forward-deklarieren, Includes in die .cpp — kappt
   die transitive §5-Kette für 10 UI-Header und senkt Rebuild-Kaskaden am
   Hotspot #4/#6. Zusätzlich `IClockSource.h`-Include prüfen (ungenutzt?).
2. **ChassisSchema nach Core verschieben** (`Core/ChassisSchema.h`, ohne
   `ConduitModule.h`-Include): entfernt 5 UI→Modules- und 1 Core→Modules-Kante
   und macht das FX-Schema für generische UIs (CLAP) verfügbar. Rule
   `fx-chassis` + docs/FxChassis.md anpassen.
3. **ADR „UI-Realtime-Datenpfad"**: einheitliches Tap-Muster (Core-Objekt mit
   SPSC/atomic, vom Modul befüllt, von UI gelesen) als Ersatz für
   `dynamic_cast`-Modulzugriffe in `ScopeDisplay`, `StepGridDisplay`,
   `GainFaderMeter`, `NodeComponent` — Voraussetzung für Mixer-Page und CLAP.
4. **Modul-Metadaten-Interface für Core-Dienste**: `OscAddress`, `OscController`,
   `OscSendService`, `PageManager`, `SignalFlowColours`, `SourceNameResolver`,
   `ConduitMacroTargets`, `RemoteModuleBinder` von `ConduitModule.h` auf ein
   schmales Interface (oder Forward-Decl) umstellen — löst 4.2 auf.
5. **CLAUDE.md §13.5 + Layering-ADR nachziehen**: `Source/DSP` und
   `Source/TouchLive` dokumentieren, Kompositionsrolle von Core (EngineProcessor/
   EngineEditor/GraphManager/Main) und das hier bestätigte Soll-Layering als
   ADR festhalten, damit künftige Audits einen normativen Maßstab haben.
6. **Include-Hygiene-Sweep** (nachrangig): die 71 Forward-Decl-Kandidaten aus
   5.2 abarbeiten (v. a. `GridPage.h`, `MacroPanel.h`, TouchLive-Views,
   `ScaleQuantizer`-/`IClockSource`-Leichen) — reine Compile-Zeit-Maßnahme,
   je Änderung kompilierend verifizieren.

---

*AUDIT-05 abgeschlossen 20.07.2026 — read-only, keine Quelldatei geändert.*
