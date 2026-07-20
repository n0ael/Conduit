# AUDIT-07 — Test-Gap-Analyse (Catch2-Abdeckung)

> Read-only-Abgleich Tests ↔ Pflichtliste §13.4 + Rule-Invarianten ·
> Referenz CLAUDE.md v5.6 (Version-Check bestanden) · 20.07.2026 ·
> Branch: master (b1f998c) · Querverweise: AUDIT-02 (TSan), AUDIT-04 (ASan)
>
> Pflichtlektüre erfüllt: §13.4/§13.5, alle 12 Rules, Dossiers
> NodeEditor/MidiRig/Looper/LinkAudio (jüngste Subsysteme).

---

## 1. Zusammenfassung

Die Testbasis ist beachtlich: **186 Testdateien, 1179 TEST_CASEs**
(Core 734 · DSP 166 · UI 210 · TouchLive 56 · Util 13), und die
§13.4-Pflichtliste ist zu **3 von 4 Punkten** erfüllt — der vierte
(CalibrationProfile-Matching) ist eine *begründete* Lücke, weil das
CV-Subsystem selbst noch Roadmap ist (AUDIT-03: tote Rule-Pfade). Die
im Auftrag genannten Bug-Klassen sind überraschend gut abgedeckt:
**alle vier haben präzise Regressionstests** — inklusive des vom
LinkAudio-Dossier geforderten Jitter-Servo-Tests und der
Mopho-`nrpn_max_value`-Lektion („min/max aus dem Profil begrenzen das
Mapping"). Der „offene NRPN-Hardware-Bug ~halber Fader-Weg" ist laut
docs/MidiRig.md **aufgeklärt und kein Conduit-Fund** (Heat =
Geräte-Firmware; Mopho = Profil-Datenfehler, behoben + getestet).

Die echten Lücken liegen an drei Stellen: (1) die **jüngste Migration
(ADR 013, `normalizeLoadedNodes`: looper_in/looper_big_out →
looper_patch_in/out, looper_out → nodeError) hat keinen einzigen
Test**, (2) der **Feldtest-Fix vom 19.07.2026** (Registry-synchrones
Re-Arming gegen stale Looper-Quell-Indizes) ist nicht als Regression
festgehalten, (3) ein expliziter **§6-Negativtest** („keine
Laufzeit-IDs im Serialisat") fehlt. Dazu kommen die aus AUDIT-02/04
bekannten Sanitizer-Lücken (Abschnitt 5).

---

## 2. Abgleich Pflichtliste §13.4

| Pflichtpunkt §13.4 | Vorhandene Tests | Lücke? |
|---|---|---|
| **SPSC-Queues** | `Tests/Util/SpscQueueTests.cpp` (echte Threads, TSan-wirksam) + Protokoll-Konsumenten: `LinkReceiveStreamTests` („Producer/Consumer-Stress über echte Threads (TSan)"), `LooperBankTests` („nebenläufige Commits gegen laufendes Audio"), `CaptureStressTests`, `ThreadingStressTests` | **Nein** |
| **ValueTree-Serialisierung/-Migration** | `PresetTests`, `AudioEndpointTests` („Migration: Alt-Patch erhält Default-I/O + Version 3", „V3-Patch ohne Output bleibt ohne — kein Auto-Repair"), `PageManagerTests` (Pages-Migration + Idempotenz + „Migration ohne Undo-History"), `GridSessionStoreTests` (7 Roundtrips inkl. stripLayer-Kollisions-Lektion), `LooperSettingsTests`, `CaptureSettingsTests`, `ChannelNamesTests`, `MidiRigSettingsTests` | **Teilweise**: die ADR-013-factoryId-Migration (`GraphManager::normalizeLoadedNodes`) hat KEINEN Test (→ Abschnitt 4/G1) |
| **Graph-Topologie (Fade-Zyklen, Batch-Coalescing)** | `GraphManagerTests` („Batch-Coalescing: … ein Rebuild", „Preset-Load: … ein Rebuild", „Graph-Swap: vollständiger Fade-Zyklus nach CLAUDE.md 5.2", „gestopptes Audio blockiert den Swap nicht", zweiphasiges Delete ×2, Async-Prepare-nodeError ×2), `GraphFaderTests`, `ThreadingStressTests` (Fade nebenläufig) | **Nein** |
| **CalibrationProfile-Matching** | — (weder Code noch Tests: `*Calibrat*`/`*CVTuner*`/`*HardwareIO*` = 0 Dateien) | **Ja, begründet**: Subsystem ist v2.0-Roadmap; Pflichtpunkt wird mit der Implementierung fällig (Rule calibration + docs/Calibration.md liegen bereit) |

§13.5-Spiegelung: siehe AUDIT-03 D3 (kein `Tests/Interfaces`
[header-only Kontrakte], kein `Tests/Modules` [Modul-Tests leben in
`Tests/Core`]) — für die Gap-Analyse ohne Abdeckungsfolgen: jedes
Modul mit Logik hat eine Testdatei in Tests/Core.

---

## 3. Invarianten-Abdeckung je Rule

Bewertung: ✓ = Invariante hat gezielte Tests · (✓) = implizit/teilweise
· ✗ = testbar, aber ungetestet · n/a = nicht sinnvoll testbar bzw.
Subsystem fehlt.

| Rule | Invariante | Abdeckung |
|---|---|---|
| patch-engine | 4-Schritt-Swap, Fade-Zyklus, kein Busy-Poll | ✓ GraphManagerTests/GraphFaderTests |
| patch-engine | Zweiphasiges Delete + Zombie-UI-Schutz (Registry) | ✓ GraphManagerTests, NodeCanvasTests („UI blockiert Phase 2 bis zur Freigabe"), FxModulePanelTests |
| patch-engine | Batch-Coalescing (Undo/Preset/Bulk) | ✓ GraphManagerTests |
| patch-engine | `nodeError` statt Crash/Retry | ✓ GraphManagerTests (2 Cases) |
| patch-engine | isDirty-Guard Preset-Save (6.1) | ✓ OscControllerTests/PresetTests (`flushPendingUpdates`); Undo-Snapshot-Seite ungetestet — deckungsgleich mit AUDIT-02 V2 |
| patch-engine | Gefadete Re-Materialisierung (5.7) | (✓) LooperPatchIn-/PatchOutModuleTests + GraphManagerTests; kein dedizierter „preparedModules verworfen"-Case |
| fx-chassis | Chassis-Kontrakt (Gains/CV/processCore), Schema-Rollen | ✓ ProcessorChassisTests, AirwindowsModuleTests |
| fx-chassis | kein rand()/Heap im Prozesspfad, deterministische Seeds | ✓ 58 DSP-Testdateien + RT-Audit-Sections (ScopedRealtimeSection in Bar/Capture/Looper/WaveformTap-Tests); Dither-Statistik in LinkAudioSendTests |
| looper | Playhead jitter-frei, Snap-Duck, kein rohes beatAtBlockStart | ✓ LooperBankTests („Wall-Clock-Jitter erreicht den Lesekopf nicht", „Beat-Achsen-Sprung re-synct klickfrei", „kurzer Spike OHNE Re-Sync") |
| looper | Retire-Protokoll, free nie im Callback, RT-Audit | ✓ LooperBankTests (Retire, „process ist allocation-free") |
| looper | Grid-Übertritte sample-genau (gridCrossingOffset, FP-Epsilon) | ✓ LooperClipMathTests + LooperBankTests (quantisierter Start/Stop/Retrigger/Reset) |
| looper | Busse/AudioView/meterChannelOf/globalTrackNumber (ADR 012) | ✓ LooperBankTests (I/O-Busse, trackBus post-fader) + LooperPatchOutModuleTests (meterChannelOf/globalTrackNumber) |
| looper | Quell-Schlüssel-Auflösung + Arming-Refcount | ✓ LooperSourceTests (3 Cases inkl. Link-Receive-als-Tap) |
| looper | **Registry-SYNCHRONES Re-Arming (Feldtest-Fix 19.07.: Re-Mat ⇒ neue Slots, stale Indizes = Stille)** | **✗** kein Test referenziert `applyLooperSourceArming`/`onRegistryChanged`-Re-Mat-Szenario (→ G2) |
| looper | **ADR-013-Migration (normalizeLoadedNodes, Alt-Keys, looper_out→nodeError)** | **✗** (→ G1) |
| looper | Papierkorb (Detach, entryId, Restore, clearWithoutDelete) | ✓ LooperTrashTests |
| linkaudio | int16+TPDF-Dither (LCG), Samples≠Frames | ✓ LinkAudioSendTests (Dither-Statistik), LinkSendTapsTests |
| linkaudio | WeakReference-Pflicht / Teardown-Race | ✓ LinkAudioSend-/ReceiveTests, ProcessorChassisTests (`releaseSessionResources`); Ausnahme GraphManager-Rohhalter ungetestet (AUDIT-04 V1) |
| linkaudio | Beat-Alignment + Servo (kein Jitter-Re-Pitch) | ✓ LinkReceiveStreamTests — der vom Dossier geforderte Regressionstest existiert wörtlich |
| linkaudio | **ChannelKeys nie serialisieren (§6), nur targetPeer/targetChannel** | (✓) createState-Schema-Test prüft die Wunsch-Properties; **kein Negativtest** „kein ChannelKey im Serialisat" (→ G3) |
| midirig | 1 SpscQueue pro Port, NRPN VOR Push, Audio nie beteiligt | ✓ MidiPortHubTests, NrpnAssemblerTests; threaded fehlt (→ Abschnitt 5) |
| midirig | Latest-Pending-Überlauf | ✓ MidiPortHubTests (overflowSlot/drainNow) |
| midirig | SysEx armed-gated, nur komplette Nachrichten | ✓ MidiPortHubTests, DsiSysexTests, HardwarePresetScannerTests (inkl. cancel/Timeout/Retry) |
| midirig | rel_encoding profil-getrieben (M8.1-Lektion beide Richtungen!) | ✓ ControllerProfileTests/MidiInBindingsTests/ChannelStripLayersTests (RelativeEncoding/signBit) |
| midirig | PB = 128+Kanal, AddressModes, PositionFeedbackRouter touch-gated | ✓ MidiInBindingsTests, PositionFeedbackRouterTests |
| midirig | `stripLayer` nie `layer` (Kollisions-Lektion) | ✓ GridSessionStoreTests („Channelstrip-Ebenen Roundtrip") |
| midirig | Pickup/LED-Router (isManaging, Echo-Restore) | ✓ PickupLedRouterTests, MidiInBindingsTests |
| grid | Voice-Modell, Achsen-Klemmung am Ausgang (ADR 003) | ✓ GridVoiceEngineTests, ExpressionAxisTests (14 Cases, alle combined()-Clamps), MpeEncoder-/MpeMidiSink-/VoiceAllocatorTests |
| transport | Metronom sample-genau, Launch-Quant-Muster 4.5 | ✓ MetronomeTests, LaunchQuantizationTests, ClockTests, TransportLinkTests (poll-basiert, Start/Stop-Sync-Regel) |
| osc-remote | Dual-State, Registry-Lebensdauer, isDirty | ✓ OscControllerTests (echte Threads), NamedIdTests |
| osc-remote | Send-Diff 30 Hz, float-Vergleich, Echo-Impfung | ✓ OscSendServiceTests |
| osc-remote | Announce/remoteId-Format | ✓ RemoteModuleBinderTests |
| touchlive | Diff/Chunks/Heartbeat/Echo-Suppression/Thinning | ✓ TouchLiveClientTests, LiveSetModelTests, LiveRemoteBridgeTests, AlphaTrackLcdTests, LiveFaderScaleTests, LiveSpectrumTapTests (56 Cases) |
| node-editor | Seiten = View-Schicht, setProperty-Regel, Regel a, activePage | ✓ PageManagerTests (6 Cases inkl. „reines setProperty — kein ChildRemoved") |
| node-editor | Canvas-Filter, Viewport-Transform/-Persistenz, Interaktions-Sperre | ✓ NodeCanvasTests (21 Cases), CanvasViewportTests (inkl. GestureRecognizer-Zustandsmaschine) |
| node-editor | Gesten-Leiter (Pinch-Dead-Zone, EMA, Ebenen 3–5) | (✓) CanvasViewportTests deckt den Recognizer — Tiefe der Gesten-Ebenen-Fälle nicht einzeln verifiziert; **Feldtest ohnehin offen** (M0–M4) |
| ui-design | minimumHorizontalScale, Push-Design | ✓ PushDesignTests; UiFramePacer selbst ohne dedizierte Tests (nur UiSettingsTests-Umfeld) — klein |
| calibration | alle Invarianten | n/a (Subsystem nicht implementiert) |

---

## 4. Regressionstest-Lücken zu bekannten Bug-Klassen

| Bug-Klasse (Auftrag) | Status | Befund |
|---|---|---|
| **NRPN-MSB/LSB-Skalierung („~halber Fader-Weg")** | ✓ abgedeckt + aufgeklärt | Laut docs/MidiRig.md M2-Feldtest ist der Bug **kein Conduit-Fund**: Heat = Geräte-Firmware (Wraparound nahe Max), Mopho = falsches `nrpn_max_value` im Profil (Daten, nicht Code). Regressionstests existieren: „MidiNrpnTarget: Sequenz 99/98/6/38, Mapping und 14-bit-Dedupe" und **„min/max aus dem Profil begrenzen das Mapping"** (exakt die Mopho-Lektion) + NrpnAssembler-Empfangsseite. Kein Handlungsbedarf im Code; ggf. CSV-Lint (Folge F6) |
| **ExpressionAxis `combined()`-Clamping** | ✓ abgedeckt | 14 TEST_CASEs decken alle Klemm-Fälle: [outMin,outMax], Kurven-Max/-Min bei Extrapolation, invertierte Kurve (Min > Max), Offset-Grenzen, offsetBeyondMax ± inkl. Kapazitäts-Klemme |
| **Looper-Statemachine** | ✓ abgedeckt | 25+ LooperBankTests: Commit/Re-Commit/Retire, Pending-Aktionen sample-genau, VARI/Reverse/×2÷2, Mix, Fehlerfälle, RAM-Budget, 16-Track-Stress, RT-Audit, nebenläufiger Stress |
| **ValueTree-Migration rootStateVersion 3 (Stereo-I/O-Default)** | ✓ abgedeckt | AudioEndpointTests: Alt-Patch → Default-I/O + Version 3; V3 ohne Output bleibt ohne (kein Auto-Repair) |
| **Session-transiente IDs (§6)** | **Lücke (G3)** | Es gibt keinen Negativtest, der ein Serialisat auf Laufzeit-IDs prüft. Receive persistiert korrekt nur targetPeer/targetChannel (Schema-Test), aber „ChannelKey/remote Laufzeit-IDs tauchen NIE im XML auf" ist nirgends behauptet — genau die v1-Phantom-Connection-Klasse |
| **NEU identifiziert: ADR-013-Migration** | **Lücke (G1)** | `normalizeLoadedNodes` (looper_in→looper_patch_in, looper_big_out→looper_patch_out, looper_out→nodeError-Pfad; tap:-Keys bleiben) — jüngste Migration (19.07.2026), 0 Tests; Alt-Patches sind das klassische Feld-Risiko |
| **NEU identifiziert: Stale-Looper-Quellen (Feldtest 19.07.)** | **Lücke (G2)** | Der Fix (CaptureService::onRegistryChanged → applyLooperSourceArming, Re-Mat registriert NEUE Slots) hat keinen Regressionstest — das Symptom war „dauerhaft Stille bei unverändertem Combo-Eintrag" |

---

## 5. Sanitizer-Lücken (Querverweis AUDIT-02 §5 / AUDIT-04 §6)

TSan/ASan laufen in CI bei jedem Push, sehen aber nur, was Tests
nebenläufig bzw. überhaupt ausführen. Ungedeckt bleiben:

1. **MidiPortHub** MIDI-System-Thread → MT (Overflow-Slot-Flut,
   SysEx-Arming-Toggle, Port-Close während Flut) — single-threaded
   getestet (AUDIT-02 Nr. 1).
2. **LiveSpectrumTap** Producer-Wechsel Audio→Link — das
   Zwei-Producer-Fenster (AUDIT-02 V1) hat keinen threaded Test.
3. **ScopeModule / LooperWaveformTap** Audio→UI-Drains —
   single-threaded (AUDIT-02 Nr. 3).
4. **CaptureWriter**-Writer-Thread (Report/Overrun) — Seam-getrieben
   (AUDIT-02 Nr. 4).
5. **VBlank→callAsync-Phase-2-Hop** — headless nie gefeuert, Tests
   rufen `completeTeardownNow()` direkt (AUDIT-04 ASan-Lücke 1).
6. **GraphManager::linkClock-Rohhalter** — kein Test setzt je eine
   Clock am GraphManager (AUDIT-04 V1).
7. **EngineProcessor::releaseResources**-Consumer-Wechsel — nur
   jassert-gesichert, bewusst („Audio steht"), kein nebenläufiger Test.

---

## 6. Priorisierte Testwunschliste (Risiko × Aufwand)

Jeder Eintrag taugt als eigener Folgeauftrag; Reihenfolge =
Empfehlung.

| # | Test | Risiko | Aufwand | Begründung |
|---|---|---|---|---|
| **G1** | ADR-013-Migrationstests: Alt-Patch mit `looper_in`/`looper_big_out`/`looper_out` laden → factoryIds normalisiert, Kabel/moduleIds/tap:-Keys erhalten, looper_out → nodeError | **hoch** | **klein** | jüngste Migration, 0 Abdeckung, klassisches Alt-Patch-Feldrisiko; Rig existiert (GraphManagerTests/PresetTests-Muster) |
| **G2** | Regressionstest Stale-Looper-Quellen: gearmter tap:-Key → Looper-patch-IN re-materialisieren → Auflösung zeigt auf die NEUEN Slots (kein Stille-Fall) | **hoch** | **mittel** | frischer Feldtest-Bug (19.07.), Hook-Kette CaptureService↔EngineProcessor ungetestet |
| **G3** | §6-Negativtest: Preset-/State-XML eines Patches mit Link-Receive + M4L-Node enthält KEINE ChannelKeys/Laufzeit-IDs (Whitelist: remoteId als dokumentierte Ausnahme) | mittel | **klein** | verhindert die v1-Phantom-Connection-Klasse dauerhaft per Schema-Wächter |
| **T1** | Threaded MidiPortHub-Test (Producer-Thread + 60-Hz-Drain: Flut/Overflow, SysEx-Arming, Port-Close) | **hoch** | mittel | größte TSan-Lücke (AUDIT-02 F2); Seams existieren |
| **T2** | Threaded LiveSpectrumTap-Moduswechsel-Test (deckt das V1-Fenster aus AUDIT-02) | mittel | klein | belegt/erzwingt den Fix des Zwei-Producer-Fensters |
| **T3** | VBlank-Hop-Seam-Test (beginTeardown → VBlank-Callback manuell feuern → SafePointer-Pfad unter ASan) | mittel | klein | AUDIT-04 ASan-Lücke 1 |
| **T4** | Threaded Drain-Tests Scope/LooperWaveformTap | niedrig | klein | letzter Anzeige-Pfad ohne TSan |
| **W1** | Re-Mat-Detail 5.7: laufender Swap + Slot-Umbau → vorbereitete Instanz wird verworfen (`preparedModules.erase`) | mittel | mittel | einziger ungetesteter Zweig des Swap-Protokolls |
| **W2** | Undo-Snapshot-Guard-Test (fällig NACH der V2-Entscheidung aus AUDIT-02 F3): Delete-Transaktion mit pending OSC-Wert → Undo restauriert den geflushten Wert | niedrig | klein | dokumentiert die gewählte Semantik |
| **W3** | GraphManager-mit-LinkClock-Rig (fällig mit AUDIT-04 F1 WeakReference-Umbau) | niedrig | klein | macht den Teardown-Pfad beobachtbar |
| **F6** | Profil-Daten-Lint als Test: alle Factory-CSVs parsen warnungsfrei; NRPN-Ranges plausibel (min < max ≤ 16383) | niedrig | klein | fängt die Mopho-Datenklasse zukünftig beim Build |
| **(fällig mit v2.0)** | CalibrationProfile-Matching-Tests (exakt→Prefix→Neutral, §13.4-Pflicht) | — | — | mit der CV-Subsystem-Implementierung, nicht früher |
