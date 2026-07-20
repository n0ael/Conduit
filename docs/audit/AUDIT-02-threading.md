# AUDIT-02 — Threading- & Datenfluss-Audit (Thread-Übergänge)

> Read-only-Audit aller Thread-Übergänge · Referenz CLAUDE.md v5.6
> (§3.1, §4.2, §6, §6.1, §7) + docs/DataModel.md · Durchgeführt:
> 20.07.2026 · Branch: master (b1f998c) · Vorgänger: AUDIT-01
>
> Pflichtlektüre: Rules `osc-remote`, `linkaudio`, `midirig`, `looper` —
> **alle vier vorhanden**, kein Fehl-Befund.

---

## 1. Zusammenfassung

**`AbstractFifo`: 0 Treffer im gesamten `Source/`-Baum** — die
ADR-001-Erwartung (SpscQueue als einziger Inter-Thread-Queue-Baustein)
ist vollständig erfüllt; die STOPP-Bedingung wurde nicht ausgelöst.

Die Thread-Architektur ist konsequent nach dem §6.1-Dual-State- und dem
§4.2-Injektions-Muster gebaut: 13 SpscQueue-Instanzen mit sauber
dokumentierter Producer-/Consumer-Zuordnung, 174 `std::atomic`-Vorkommen
mit expliziten Memory-Orders, Message-Thread-Marshalling ausschließlich
über AsyncUpdater/`callAsync`/Timer/VBlank. Es gibt **0 harte VERSTÖSSE**
und **3 VERDACHTSFÄLLE**: (1) ein kurzes Zwei-Producer-Fenster auf der
LiveSpectrumTap-SPSC-Queue beim Quellenwechsel Audio→Link, (2) der
§6.1-isDirty-Guard ist nur für Preset-Save/`getStateInformation`
implementiert, nicht für Undo-Snapshots, (3) der dokumentierte
RNG-Kontrakt von `IStochastic.h` (ausdrücklich `juce::Random`)
widerspricht dem §3.1-Wortlaut (inline-LCG) — das relativiert zugleich
AUDIT-01-V1 zu einem reinen Doku-Konflikt. Die TSan-Abdeckung (§13.4,
CI bei jedem Push) ist für die Audio-nahen Übergänge gut; die
MIDI-Rig-, Anzeige-Drain- und Netzwerk→MT-Übergänge haben dagegen
keinen echt-threaded Testpfad (Abschnitt 5).

Interface-Ownership-Stichproben: IClockSource/IClockSlave/IStochastic
regelkonform (je ≥ 2 Aufrufstellen verifiziert). **ISidechain und
ITouchMacro existieren nicht als Code-Interfaces** (Roadmap v2.0/v2.x;
die „ITouchMacro-Kette" lebt als MT-Kontrakt in `IVoiceSink.h` weiter) —
Ownership-Prüfung dort gegenstandslos, als Befund vermerkt.

---

## 2. Thread-Übergangs-Karte

### 2.1 SpscQueue-Inventar (Phase 1.1)

| Instanz | Producer [Thread] | Consumer [Thread] | Zweck |
|---|---|---|---|
| `EngineProcessor::oscToAudioQueue` (1024) | OscController [Netzwerk, unter registryLock] | `processBlock` [Audio]; Ausnahme: `releaseResources` [MT, nur bei nachweislich gestopptem Audio, jassert-Guard `audioCallbackActive`] | OSC-Fastpath §6.1 Pfad 1 |
| `LooperBank::commands` (1024) | LooperSessionModel/UI [MT] | `drainCommands` [Audio] | Clip-Kommandos |
| `LooperBank::retired` (1024) | Audio (Retire-Quittung) | `serviceMessageThread` [MT] | Clip-Freigabe (kein `free` im Callback) |
| `BufferPool::publishQueue` | `service()` [MT] | `tryClaimSegment` [Audio] | Segment-Publikation |
| `BufferPool::releaseQueue` | `releaseSegment` [Audio] | `service()` [MT] | Segment-Rückgabe |
| `CaptureService::retiredSets` (8) | `processInputTap` [Audio] | `drainRetiredSets` [MT] | Puffersatz-Quittung |
| `LinkReceiveStream::queue` | Link-Thread (`pushBuffer`) | `renderBlock` [Audio] | Empfangs-Slots |
| `LooperWaveformTap::queue` (8192) + `spectrumQueue` (1024) | Audio (`emitBin`/`emitSpectrumColumn`) | UI-Strip [MT, VBlank] | Waveform-/Spektral-Bins |
| `ScopeModule::scopeQueue` (4096) | `processBlock` [Audio] | Scope-UI [MT] | Anzeige-Bins |
| `LiveSpectrumTap::queue` (64) | **alternierend**: Audio (`pushAudioBlock`) ODER Link-Thread (Source-Callback) — Moduswechsel serialisiert | `drainQueueInto` [MT, 30-Hz-Timer] | EQ-Spektrum; → Verdacht V1 |
| `MidiPortHub::InputConnection::controllerQueue` (512), `noteQueue` (512), `sysexQueue` (256) — **je Port ein Satz** | MIDI-System-Thread des Ports (NRPN-Assembler VOR dem Push; SysEx nur komplett + nur armed) | `drainNow` [MT, 60 Hz] | MIDI-Rig E4/E7 (Audio NIE beteiligt) |

Ergänzende Nicht-Queue-Mechanismen: `CaptureService::pendingSet`
(atomare Mailbox MT→Audio), `MidiPortHub`-Overflow-Slot (atomares
uint64 „Latest-Pending", von beiden Seiten per `exchange` bedient —
kein SPSC-Kontrakt, daher zulässig).

### 2.2 Routen-Karte (Phase 2.1)

| Route | Mechanismus | Regelkonform? |
|---|---|---|
| UI/MT → Audio (Parameter) | `getParameterTarget()`-Atomics (Modul-Member), blockweise Snapshots im Chassis | ✓ §6.1 |
| Netzwerk → Audio (OSC-Fastpath) | `oscToAudioQueue` (SPSC); Push unter `registryLock` = Lebensdauer-Invariante der target-Pointer (OscController.h:50–56) | ✓ §6.1 Pfad 1 |
| Netzwerk → ValueTree (OSC) | `pendingTreeUpdates` (CriticalSection Netzwerk↔MT) → AsyncUpdater → `applyTreeUpdates` [MT, undo-frei]; `stateDirty`-Guard mit korrekter Reset-Reihenfolge (OscController.cpp:382–388) | ✓ §6.1 Pfad 2 (AsyncUpdater statt der callAsync-Skizze aus DataModel.md — gleichwertig; Doku-Skizze siehe Abschnitt 4) |
| MT → Audio (Graph-Swap) | GraphFader-Atomics (`requestedPhase`/`fadeComplete`) + AsyncUpdater-Self-Re-Dispatch, kein Busy-Poll/Timer | ✓ §5 / Rule patch-engine |
| MT → Audio (Looper) | `SpscQueue<ClipCommand>` + atomare Fader-/Send-/Mute-Targets | ✓ |
| Audio → MT (Looper-Retire) | `SpscQueue<LooperClip*>` + Graveyard [MT] + Drain-Guard | ✓ (kein free im Callback, Rule looper) |
| Audio ↔ MT (Capture-RAM) | `pendingSet`-Mailbox, `retiredSets`-SPSC, BufferPool-Protokoll (Zähler + 2 SPSC) | ✓ |
| Audio → UI (Meter/Scope/Waveform) | Atomics (LevelMeter/InputMeter) bzw. SPSC (Scope/WaveformTap/SpectrumTap), Konsum via UiFramePacer/VBlank/Timer [MT] | ✓ §10.0 |
| Link-Thread → Audio (Receive) | `SpscQueue<LinkReceiveSlot>` (memcpy-Slots, Beat-Filter am Producer) | ✓ Rule linkaudio |
| Link-Thread → MT (ChannelsChanged) | `MessageManager::callAsync` + `WeakReference<LinkClock>` (LinkClock.cpp:119–127) | ✓ §7.2 |
| Audio → Peers (Link-Send) | SDK-Sink-Commit [Audio, Link-Garantie]; `rtSink`-Atomic + Epoch-Handshake (`blocksProcessed`) + AsyncUpdater-Retire | ✓ §7.2 / 5.3 |
| MIDI-System-Thread → MT | SpscQueues pro Port + atomarer Overflow-Slot; 60-Hz-Drain; Port-Handle als letztes Member (Producer stirbt vor den Queues) | ✓ Rule midirig E4/E7 (Audio nie beteiligt) |
| Netzwerk (TouchLive) → MT | CriticalSection-Queue (`incomingLock`) + AsyncUpdater | ✓ Rule touchlive (Audio nie beteiligt) |
| Writer-Thread ↔ MT (CaptureWriter) | `queueLock`/`reportLock` (CriticalSection) + AsyncUpdater-Report | ✓ (explizit „kein RT-Pfad") |
| Audio → Audio, selber Callback | `ClockBus.current` und `LooperBank::getAudioView()` bewusst nicht-atomar — Schreiber und Leser im SELBEN Callback (IClockSource.h:36–39, ILooperAudioClient.h:14–19) | ✓ §4.2 (dokumentierter Kontrakt) |
| MT → Audio (Materialisierungs-Injektion) | `setClockBus`/`setRandomSeed`/`applyOutputSpecs`/`applySendConfig` NUR vor `prepareForGraph`; danach eingefroren, Struktur-Änderung = Re-Materialisierung | ✓ §4.2 / 5.2 (Absicherungs-Empfehlung siehe AUDIT-01 F2) |

### 2.3 `std::atomic`-Inventar (Phase 1.2, 174 Vorkommen, gruppiert)

| Klasse | Members (Auswahl = vollständige Member-Liste der Klasse) | Threads / Zweck |
|---|---|---|
| `SpscQueue` | `head`, `tail` (alignas 64) | Producer/Consumer-Indizes |
| `GraphFader` | `requestedPhase`, `fadeComplete`, `prepared` | MT ↔ Audio Fade-Protokoll (§5.2) |
| `EngineProcessor` | `scaleRootAtomic`, `scaleTypeAtomic`, `globalSwingAtomic`, `audioCallbackActive` | MT schreibt, Audio liest; Callback-Guard |
| `CallbackTimingMonitor` | `xruns`, `peakLoadPermille`, `loadSumPermille`, `loadBlockCount` | Audio → MT Diagnose |
| `LevelMeter`/`InputMeter` | `peakLevel[]`, `peakHoldLevel[]`, `rmsLevel[]`, `clipped[]`, `clipHoldSeconds` (+ InputMeter-Pendants) | Audio → UI Metering |
| `BarSampleAnchors` | `slots[16]` (gepackt 64-bit), `latest` (+ lock-free-static_assert) | Audio schreibt, jeder Thread liest |
| `SampleClock` | Positions-Atomic (release beim `advance`) | Audio schreibt, alle lesen |
| `CaptureService` | `pendingSet`, `invalidateRequested`, `channelArmed[]`, `anyChannelActive`, Slot-`writerActive`/`detachRequested` | MT ↔ Audio Puffersatz/Gate/Arming |
| `BufferPool` | `pendingRequests`, `allocatedBytes`, `exhausted` | Audio-Anforderung, MT-Service |
| `CaptureGate`/`CaptureChannel`/`CaptureRingBuffer` | Zustands-/Range-Atomics | Audio schreibt, MT/Writer lesen |
| `CaptureWriter`/`CaptureSettings` | Fortschritts-/Settings-Atomics | Writer/MT |
| `LinkClock` | `currentSampleRate`, `clockOffsetMicros` | MT schreibt, Audio liest |
| `LinkSendTaps` | Tap: `rtSink`, `width`, `status`; Hub: `blocksProcessed` (seq_cst Epoch) | MT-Retire ↔ Audio-Commit |
| `LinkReceiveStream` | `resetRequested`, `uiStatus`, `bufferedSeconds`, `droppedPushes`, `sequenceGaps`, `renderResets` | Link/MT ↔ Audio |
| `LooperBank` | `stopAllRequested`, `anchor`, `targetGain/Pan/SendMask/sendTapPre/effectiveMute/looperToMaster`-Matrizen, `snapCount`, `ramBytesUsed` | MT-Mixer → Audio; Diagnose zurück |
| `LooperClip` | `paramVersion`, `staged*`-Satz, `applyAtWrap`, `windowFollowsPhase`, `resetAnchorToGrid`, `displayPhase01`, `exportPins` | MT-Staging → Audio-Apply; Export-Pins |
| `LooperWaveformTap` | `sourceVersion`, `sourceLeft`, `sourceRight` | MT-Quellwahl → Audio |
| `Metronome` | `enabled`, `anchor` | MT → Audio |
| Modul-Parameter-Targets | `gainTarget` (Attenuator/LfoModule `rate`/`depth`), StepSequencer-Parametersatz + `stepTargets[64]` + `currentCellForUi`, `latencyTarget` (Receive), Slot-`gainTarget` (Send) | OSC/MT → Audio (§6.1) |
| Modul-rt-Pointer | `rtTap` (ProcessorModule), `rtService`+`rtSlots` (CaptureTap/PatchIn/Receive), `rtBank` (PatchOut), `rtSlots` (InputLinkSend), `aggregateStatus`, `bindState` | Phase-1-Trennung §5.3 |
| `ProcessorModule` | `inputGainDb`, `outputGainDb`, `dspBase[]`, `cvAmount[]`, `userRangeMin/Max[]`, `linkSource/Amount/Curve*[]` | OSC/MT → Audio Chassis |
| `AirwindowsPlugin` | `parameterValues[16]`, `ditherEnabled` | Snapshot am Blockanfang |
| `MidiPortHub` | `sysexArmed`, `overflowSlot` (je Port) | MT-Arming; MIDI-Thread ↔ MT Overflow |
| `OscController` | `stateDirty`, `syncRequested`, `learnDone` | Netzwerk → MT Guards |
| `LiveSpectrumTap` | `inputTapEnabled`, `audioSampleRate` | MT-Gate → Audio-Producer |
| `TouchLiveClient` | Verbindungs-/Seq-Atomics | Netzwerk ↔ MT |
| UI/Infra | `UiFramePacer` (fps-Limit), `RtAllocationGuard` (Violation-Zähler), Browser-Generation-Zähler | MT/Pool-Jobs |

### 2.4 MT-Marshalling-Inventar (Phase 1.3)

- **AsyncUpdater (8 Klassen):** GraphManager (Batch-Coalescing 5.5),
  OscController (Tree-Pfad), LinkClock (deferred `enableLinkAudio(false)`),
  LinkSendTaps (Retire-Handshake), CaptureService (Writer-Report),
  TouchLiveClient, TouchLive-Views (GridView/MixerView/DeviceView) —
  alle mit `cancelPendingUpdate()` im Destruktor, wo Objekt-Lebensdauer
  es verlangt.
- **`MessageManager::callAsync`:** LinkClock (ChannelsChanged, mit
  WeakReference), OscController-Doku, Browser (SearchIndex/FileScanner,
  Test-injizierbar), UI-Teardown-Pfade (NodeComponent, FxModulePanel,
  EngineEditor, PushTiles, GridPage — je mit SafePointer/shared_ptr).
- **`juce::Timer` (alle MT):** MidiPortHub 60 Hz (Drain),
  CaptureService (RAM-Wächter/AutoCalibrator), OscSendService 30 Hz,
  LiveSpectrumTap 30 Hz, LooperTrashCan 10 s, TouchLiveClient
  (Heartbeat/Thinning), EngineEditor 15 Hz, diverse UI-Status-Poller
  (10 Hz) und Long-Press-/Debounce-Timer.
- **`VBlankAttachment`:** UiFramePacer (zentral) + UI-Ticks
  (AnimatedValue, LooperWaveformStrip, MacroPanel, MpeShapingView,
  NodeComponent-Teardown Phase 2, PageOverview, TrackTabsStrip,
  GridKeyboard, CcControlLayer, EngineEditor-Playheads,
  TouchLiveGridView) — ausschließlich UI/MT.

---

## 3. Verstöße

| Datei:Zeile | Regel | Snippet |
|---|---|---|
| — | — | *keine harten Verstöße gefunden* |

(`AbstractFifo`: 0 Treffer — kritischer Befund laut STOPP-Bedingung
entfällt.)

---

## 4. Verdachtsfälle

### V1 — Zwei-Producer-Fenster auf `LiveSpectrumTap::queue` beim Quellenwechsel Audio→Link

- **Fundstellen:** `Source/TouchLive/LiveSpectrumTap.cpp:30–65`
  (`setMode`: `inputTapEnabled.store(false)` … danach
  `bindFirstAvailableChannel()`), `:156–188` (`pushAudioBlock` prüft
  das Gate nur am Blockanfang), `LiveSpectrumTap.h:117` (SPSC-Queue).
- **Analyse:** Der SPSC-Kontrakt verlangt genau EINEN Producer
  (§3.1, SpscQueue.h:21–24; Rule touchlive: „Moduswechsel stoppt erst
  die alte Quelle"). Richtung Link→Audio ist das erfüllt
  (`linkSource.reset()` stoppt den Link-Callback synchron, erst danach
  `inputTapEnabled = true`). Richtung **Audio→Link** existiert ein
  Fenster: ein Audio-Callback, der das Gate VOR dem `store(false)`
  passiert hat, pusht seinen Block noch zu Ende, während der frisch
  gebundene Link-Producer bereits pushen kann — für die Dauer eines
  Callbacks zwei nebenläufige `push()` auf einer SPSC-Queue
  (Data Race auf `head`/Slot, TSan-meldbar). Auswirkung praktisch
  klein (reine Anzeige-Daten, Fenster ≤ 1 Audio-Block, erfordert
  Moduswechsel im laufenden Betrieb), aber ein echter Kontraktbruch.
  Ein Block-Epoch-Handshake (Muster LinkSendTaps) oder ein
  Ein-Block-Delay vor dem Neubinden schlösse das Fenster.

### V2 — §6.1-isDirty-Guard deckt Undo-Snapshots nicht ab

- **Fundstellen:** Guard implementiert in
  `EngineProcessor::getStateInformation` (EngineProcessor.cpp:1183) und
  `savePreset` (:1216) via `flushPendingUpdates()` (synchron — stärker
  als das dokumentierte „einen Zyklus warten"). Die 25
  `beginNewTransaction`-Stellen (GraphManager.cpp, PageManager.cpp,
  EngineProcessor.cpp:1248) flushen dagegen NICHT.
- **Analyse:** §6.1/docs/DataModel.md verlangen den Guard für
  „Preset-Save **und Undo-Snapshot**". Praktische Auswirkung ist eng
  begrenzt, weil OSC-Tree-Writes undo-frei laufen
  (`applyTreeUpdates`, OscController.cpp:456 — `nullptr` statt
  UndoManager): Parameter-Werte selbst geraten nie in Undo-Deltas.
  Rest-Exposure: eine Transaktion, die ganze Subtrees erfasst
  (z. B. „Modul löschen"), snapshotet den Node OHNE einen noch
  ausstehenden OSC-Wert — Undo des Löschens restauriert dann den
  Stand von vor dem OSC-Update (~1 Frame Datenverlust). Entweder
  Flush vor `beginNewTransaction` zentralisieren oder §6.1 auf den
  tatsächlichen (engeren) Kontrakt präzisieren.

### V3 — Regel-Doku-Konflikt: §3.1-inline-LCG vs. `IStochastic.h`-Kontrakt (`juce::Random`)

- **Fundstellen:** `Source/Interfaces/IStochastic.h:13–18`
  („Implementierende Module besitzen ihre EIGENE RNG-Instanz
  (juce::Random) und benutzen sie ausschließlich im Audio Thread")
  vs. CLAUDE.md §3.1 („Zufall im Audio-Thread NUR via inline LCG").
- **Analyse:** Der Interface-Kontrakt sanktioniert exakt das, was
  AUDIT-01 als V1 gegen den §3.1-Wortlaut gemeldet hat
  (StepSequencerModule). Threading-seitig ist alles konform:
  Seed-Injektion auf dem MT bei der Materialisierung
  (GraphManager.cpp:1796–1800, deterministisch aus der nodeUuid;
  zweite Aufrufstelle: Tests setzen den Seed direkt), RNG-Nutzung nur
  [Audio]. Der Widerspruch liegt allein zwischen zwei normativen
  Dokumenten — eine Seite muss angepasst werden (Empfehlung F4 in
  AUDIT-01 bereits erfasst; hier präzisiert: auch IStochastic.h
  anfassen).

### Geprüft und bewusst NICHT als Verdacht geführt

- **`oscToAudioQueue`-Consumer-Wechsel** in `releaseResources`
  (EngineProcessor.cpp:978–988): formaler Seitenwechsel des
  SPSC-Consumers auf den MT, aber nur bei nachweislich gestopptem
  Audio (`JUCE_ASSERT_MESSAGE_THREAD` + jassert auf
  `audioCallbackActive`) — dokumentierter, geprüfter Sonderfall.
- **Cross-Thread ohne Queue/Atomic (Phase 2.5):** `ClockBus.current`
  und `LooperBank`-AudioView (selber Callback, dokumentiert),
  Materialisierungs-injizierte Member (`specs`/`slots`/
  `totalChannels`/Seed/ClockBus-Pointer — eingefroren vor
  `prepareForGraph`), `GraphFader::lastAppliedPhase`/
  `CallbackTimingMonitor::sampleRate` (MT nur bei stehendem Audio),
  `LooperClip`-active\*-Felder (Audio-only), EngineProcessor-MT-Felder
  (`looperArmedIndices` u. a.). Alle mit explizitem, im Code
  dokumentiertem Ownership — kein ungeregelter Zugriff gefunden.
- **MidiPortHub-Overflow-Slot:** beide Seiten nutzen `exchange` auf
  EINEM Atomic — kein SPSC-Kontrakt, Latest-Pending-Semantik
  dokumentiert (User-Entscheidung 12.07.2026). Der Konstruktor
  erzwingt den MessageManager-Singleton vor Thread-Start
  (dokumentierter TSan-CI-Fund 12.07.2026).
- **DataModel.md-6.1-Skizze:** Der Beispielcode zeigt
  `callAsync` + `isDirty.store(true)` NACH dem Queuing; die echte
  Implementierung ist strenger und korrekt (store VOR
  `triggerAsyncUpdate`, Reset VOR dem Drain — OscController.cpp:375,
  382–388). Reine Doku-Kosmetik, kein Code-Befund (Folgeauftrag F6).

---

## 5. Abgleich mit TSan-Abdeckung (§13.4)

TSan läuft in CI (Ubuntu, jeder Push auf master) über das komplette
`ConduitTests`-Target — Races sind aber nur dort sichtbar, wo Tests
ECHTE Threads treiben. Bestandsaufnahme (grep nach
`std::thread`/`startThread` in `Tests/`):

**Echt-threaded (TSan-wirksam) abgedeckte Übergänge:**

| Übergang | Testdatei |
|---|---|
| SpscQueue Producer/Consumer | `Tests/Util/SpscQueueTests.cpp` |
| Graph-Swap MT ↔ Audio (Fade-Protokoll) | `Tests/Core/ThreadingStressTests.cpp` (GraphFader), `GraphFaderTests.cpp` |
| Parameter-Atomics MT → Audio | `ThreadingStressTests.cpp` (Attenuator `setGain` vs. `processBlock`) |
| BarSampleAnchors Audio-Write vs. Fremd-Lookup | `BarSampleAnchorsTests.cpp` |
| Capture-Ring/Pool/Puffersatz | `CaptureBufferTests.cpp`, `CaptureStressTests.cpp` |
| Link-Send (Taps/Epoch-Retire) | `LinkAudioSendTests.cpp`, `InputLinkSendTests.cpp`, `LinkSendTapsTests.cpp` |
| Link-Receive (Link-Thread → Audio) | `LinkReceiveStreamTests.cpp` |
| Looper-Kommando/Retire | `LooperBankTests.cpp` |
| OSC Netzwerk → Audio/Tree | `OscControllerTests.cpp` |
| M4L-Announce-Binder | `RemoteModuleBinderTests.cpp` |

**OHNE echt-threaded Testpfad (TSan sieht diese Übergänge nie unter
Nebenläufigkeit):**

1. **MidiPortHub** (MIDI-System-Thread → MT): `MidiPortHubTests.cpp`
   treibt Producer und Drain single-threaded über die injizierten
   Öffner — Overflow-Slot-Protokoll, SysEx-Arming und
   Port-Close-Reihenfolge (Handle-Destruktor vs. Queue-Lebensdauer)
   sind nie nebenläufig getestet.
2. **LiveSpectrumTap** (Audio/Link-Producer ↔ MT-Drain,
   Producer-Wechsel): `LiveSpectrumTapTests.cpp` ohne Threads —
   ausgerechnet das V1-Fenster hat keinen TSan-Pfad.
3. **ScopeModule** und **LooperWaveformTap** (Audio → UI/MT-Drain):
   Tests single-threaded.
4. **CaptureWriter** (Writer-Thread ↔ MT Report/Overrun-Pfad):
   `CaptureWriterTests.cpp` ohne eigene Threads (Seam-getrieben).
5. **TouchLiveClient** (Netzwerk → MT): Seam-getrieben,
   single-threaded (Audio ist hier per Architektur nie beteiligt —
   Risiko entsprechend geringer).
6. **EngineProcessor::releaseResources**-Consumer-Wechsel: nur per
   jassert zur Laufzeit geprüft, kein nebenläufiger Test (bewusst —
   der Kontrakt IST „Audio steht").

---

## 6. Empfohlene Folgeaufträge (priorisiert)

1. **(hoch)** LiveSpectrumTap-Producer-Wechsel absichern (V1):
   Block-Epoch-Handshake nach LinkSendTaps-Muster oder verzögertes
   Neubinden; dazu ein echt-threaded TSan-Test (Producer-Thread +
   Moduswechsel im Loop).
2. **(mittel)** Echt-threaded TSan-Tests für MidiPortHub: simulierter
   MIDI-Producer-Thread gegen 60-Hz-Drain, inkl. Overflow-Flut,
   SysEx-Arming-Toggle und Port-Close während laufender Flut.
3. **(mittel)** V2 entscheiden: zentralen Flush vor
   `beginNewTransaction` (ein Helper im GraphManager/PageManager) ODER
   §6.1/DataModel.md auf den realen Kontrakt präzisieren („Undo sieht
   nur Basiswerte; Subtree-Snapshots flushen vor Delete-Transaktionen").
4. **(mittel)** Threaded Drain-Tests für ScopeModule/LooperWaveformTap
   (Audio-Producer-Thread gegen UI-Drain) — billig, schließt die
   letzten Anzeige-Pfade.
5. **(niedrig)** V3/AUDIT-01-V1 auflösen: §3.1 und `IStochastic.h`
   widersprechen sich — eine Quelle der Wahrheit festlegen (Empfehlung:
   §3.1 um „bzw. modul-eigene `juce::Random`-Instanz per
   IStochastic-Kontrakt" ergänzen ODER IStochastic auf den inline-LCG
   umschreiben und StepSequencer anpassen).
6. **(niedrig)** DataModel.md-6.1-Codeskizze an die echte
   (strengere) Implementierung angleichen: AsyncUpdater +
   `pendingTreeUpdates` statt roher `callAsync`-Lambda,
   Dirty-Reihenfolge „store(true) VOR triggerAsyncUpdate / Reset VOR
   Drain".
7. **(niedrig)** ISidechain/ITouchMacro: beim ersten implementierten
   Vertreter die §4.2-Ownership-Zeile direkt in den neuen Header
   übernehmen (Muster `IStochastic.h`/`IVoiceSink.h`), damit die
   Ownership-Prüfung künftiger Audits greifbar ist.
