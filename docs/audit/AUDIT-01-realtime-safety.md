# AUDIT-01 — Realtime-Safety-Audit (CLAUDE.md §3.1)

> Read-only-Audit aller Audio-Thread-Pfade · Referenz CLAUDE.md v5.6
> Durchgeführt: 20.07.2026 · Branch: master (b1f998c)
>
> Pflichtlektüre vor Phase 1: CLAUDE.md §3, §4.2, §6.1; Rules
> `patch-engine`, `fx-chassis`, `looper`, `linkaudio` — **alle vier
> vorhanden**, kein Fehl-Befund.

---

## 1. Zusammenfassung

Der Audio-Pfad von Conduit Alpha v3 ist in ausgezeichnetem Zustand: Das
Audit fand **0 VERSTÖSSE** und **2 VERDACHTSFÄLLE** (beide funktional
RT-sicher, aber Abweichungen vom Regel-Wortlaut bzw. vom dokumentierten
Thread-Kontrakt). Alle geprüften Heap-, Lock-, String-, ValueTree-,
Zufalls-, Zeitbasen- und OS-Call-Kategorien sind in den Audio-Pfaden
sauber; Allokationen und Locks liegen ausnahmslos in Konstruktor-,
prepare-, Message-Thread-, Netzwerk- oder Writer-Thread-Kontexten mit
dokumentierter Thread-Ownership. Die Architektur setzt die §3.1-Muster
konsequent um (SpscQueue als einziger Queue-Baustein, Atomics mit
expliziten Memory-Orders, Puffersatz-Mailboxen, Retire-/Epoch-Handshakes,
inline-LCG-Dither) und bringt mit `RtAllocationGuard` sogar eine eigene
Audit-Infrastruktur mit. Zwei dokumentierte, begründete Ausnahmen zu §3.1
(CallbackTimingMonitor-QPC, Link-SDK-chrono-Zeitbasis) sind als solche
gekennzeichnet und verifiziert unkritisch.

Inventur-Randnotiz: Die Erwartung „alle Treffer unter
`Source/{Core,Modules,Util}`" wird zweifach formal verfehlt —
`Source/Interfaces/` (nur Doku-Kommentare in Headern, keine
Implementierungen) und transitiv `Source/TouchLive/LiveSpectrumTap`
(Audio-seitiger `pushAudioBlock`, geprüft sauber). Die STOPP-Bedingung
(Implementierungen unter `Source/UI/`) wurde **nicht** ausgelöst.

---

## 2. Verstöße

| Datei:Zeile | Kategorie | Snippet | betroffene MD-Regel |
|---|---|---|---|
| — | — | *keine Verstöße gefunden* | — |

---

## 3. Verdachtsfälle mit Begründung

### V1 — `juce::Random` im Audio-Pfad des Step-Sequencers

- **Fundstellen:** `Source/Modules/StepSequencerModule.cpp:203` und
  `:206` (`random.nextFloat()`, `random.nextInt (chainLength)` in
  `renderChain`, erreichbar aus `processBlock`); Member
  `Source/Modules/StepSequencerModule.h:120` (`juce::Random random`).
- **Regel:** CLAUDE.md §3.1 (Ergänzung): „Zufall im Audio-Thread NUR via
  inline LCG (state = 1664525\*state + 1013904223), nie
  rand()/\<random\>-Engines".
- **Begründung der Einstufung als VERDACHT (nicht VERSTOSS):**
  `juce::Random` ist weder `rand()` noch eine `<random>`-Engine, sondern
  intern ein 64-Bit-LCG — heap- und lock-frei, `noexcept`-Pfade,
  deterministisch pro Seed (Seed-Injektion via `setRandomSeed`
  [Message Thread, vor Graph-Aufnahme], IStochastic-Kontrakt §4.2 wird
  eingehalten). Die *Intention* der Regel (kein Heap, kein Lock,
  Determinismus) ist erfüllt; der *Wortlaut* (genau dieser eine
  inline-LCG) nicht. Zum Vergleich: LinkSendTaps (TPDF-Dither,
  `LinkSendTaps.cpp:278–281`) und die Airwindows-fpd-Seeds nutzen
  regelkonform den inline-LCG bzw. deterministische Xorshift-Seeds.
- **Risiko:** kein Laufzeitrisiko; Inkonsistenz-Risiko, falls künftige
  Module dem Beispiel folgen und dabei RT-unsichere Random-Quellen
  einschleppen.

### V2 — `AirwindowsPlugin::setParameter` vom Audio-Thread trotz [Message Thread]-Doku

- **Fundstellen:** `Source/Modules/AirwindowsProcessorModule.cpp:43–44`
  (`plugin->setParameter (i, effectiveParam (i))` in `processCore`,
  also [Audio Thread]); Kontrakt-Doku
  `Source/DSP/Airwindows/AirwindowsPlugin.h:72–73`
  („// [Message Thread] … void setParameter (…)").
- **Begründung:** `setParameter` schreibt `noexcept` in ein
  `std::atomic<float>` (relaxed) — technisch RT-sicher, kein Heap, kein
  Lock. Der Aufruf verletzt aber die dokumentierte Thread-Ownership der
  Klasse (§4.2: „Niemals Interface-Methoden vom falschen Thread
  aufrufen"). Entweder ist die Header-Annotation zu eng oder der
  Chassis-Aufruf gehört umgebaut (direkter Snapshot-Übergabepfad).
- **Risiko:** kein Laufzeitrisiko heute; Kontrakt-Drift — wer der
  Header-Doku vertraut, könnte `setParameter` später um MT-only-Logik
  erweitern (z. B. String-Notify) und damit unbemerkt den Audio-Pfad
  brechen.

---

## 4. Explizit geprüfte und saubere Bereiche

**Inventur (Phase 1).** 36 Dateien mit `processBlock`, 4 mit
`processCore`; transitiv erreichte projektinterne Helfer vollständig
erfasst (Includes eine Ebene, Verdachtsfälle tiefer verfolgt). Keine
Implementierung unter `Source/UI/`.

**Zentraler Callback** — `EngineProcessor::processBlock`
(`EngineProcessor.cpp:991–1150`): SPSC-Drain der OSC-Queue
(`update.target->store`, Pointer-Lebensdauer als harte Invariante
dokumentiert, `OscController.h:50–56`), ClockBus-Fill aus Atomics,
`rt::ScopedRealtimeSection`-Audit-Inseln um alle Engine-Bausteine,
`audioCallbackActive`-Guard für den SPSC-Consumer-Wechsel in
`releaseResources`. Sauber.

**Looper-Engine** — `LooperBank` (renderBlock/mixToOutput/getAudioView/
drainCommands/executePending/launchVoice/handleActivate/handleDelete/
transferOrRetire/computeCandidate/applyClipParams/renderClipSample,
`LooperBank.cpp:618–1414`): feste Busse
(`FloatVectorOperations::clear/add`), `SpscQueue<ClipCommand>` MT→Audio,
Retire-Queue Audio→MT mit Drain-Guard (kein `free` im Callback,
Graveyard-`push_back` nur im MT-Pfad `moveToGraveyard`), Beat-Messung
jitter-frei aus `BarSampleAnchors` + SampleClock (Wall-Clock-Lektion
umgesetzt), Stack-`AudioBuffer`-View für Meter alloc-frei
(preallocated channel space). Sauber.

**Capture-Fundament** — `CaptureService::processInputTap` /
`writeVirtualChannel` / `openGate` / `closeGate`
(`CaptureService.cpp:287–601`): Puffersatz-Mailbox
(`pendingSet.exchange`), Retire über SPSC, `SampleClock::advance` als
letzte Operation (Release-Kontrakt). `BufferPool` (MT besitzt alle
Segmente; Audio-Seite wait-free: `requestSegment` = atomarer Zähler,
`tryClaimSegment`/`releaseSegment` = SPSC). `CaptureChannel`,
`PreRollBuffer`, `CaptureGate`, `InputMeter`: keine Treffer in
irgendeiner Verbots-Kategorie. Alle `make_unique`/`push_back` in
`buildSet`/`enqueueExport` etc. sind Message-Thread (Registry, Export-
Job-Bau); `reportLock` ist Writer→MT und explizit „kein RT-Pfad"
kommentiert. Datei-I/O ausschließlich im `CaptureWriter` (dedizierter
Writer-Thread, `queueLock` Writer↔MT).

**Link-Schicht** — `LinkClock::captureClockState`/`Sink::
commitFromClockState` (`LinkClock.cpp:284–388`): lock-free per
Link-SDK-Garantie, `std::optional`-Stash mit Inline-Storage
(dokumentiert allokationsfrei), `std::copy_n` in SDK-Buffer.
`std::chrono::microseconds` (`LinkClock.cpp:186/379`) ist die
**Zeitbasis des Link-SDK selbst** und die Quelle des ClockState — keine
verbotene Misch-Mathematik, sondern die §3.1-konforme musikalische
Zeitbasis an ihrer einzigen Entstehungsstelle. `LinkSendTaps::Tap::
commit` + `convertToInt16Tpdf` (inline-LCG-Dither, §3.1-konform);
`getMillisecondCounter` (`LinkSendTaps.cpp:201/246`) liegt im
MT-Retire-Handshake (`JUCE_ASSERT_MESSAGE_THREAD` + AsyncUpdater).
`InputLinkSend::processBlock`: atomare rtSlots, defensive
Kanal-Dopplung. `LinkReceiveStream`: Ring einmalig im Ctor, Producer =
Link-Thread, Consumer = Audio (SPSC), Catmull-Rom/Servo rein
arithmetisch. Sauber.

**FX-Chassis** — `ProcessorModule::processBlock`
(`ProcessorModule.cpp:276–406`): CV-Blockmittel, zweistufige
Effektiv-Werte, `SmoothedValue`-Ramps, `setDataToReferTo` (alloc-frei),
Tap-Commit mit Epoch-Handshake. Airwindows-Basis
(`AirwindowsPlugin.h`): Block-Snapshot aus Atomics, fpd-Xorshift
deterministisch geseedet; alle 114 Plugin-Ports frei von
new/malloc/String/rand (grep-verifiziert; Delay-Puffer als feste
Klassenmember, PORTING_NOTES dokumentiert die TapeDust-Sanierung).

**Modul-processBlocks** (alle gelesen): `AttenuatorModule`,
`ScopeModule` (SPSC-Bins), `LfoModule`, `CaptureTapModule`,
`LooperPatchInModule`, `LooperPatchOutModule`, `AudioEndpointModule`
(No-op-Pass-Through), `LinkAudioSendModule` (Scratch in `prepareToPlay`
vorallokiert, Guard gegen zu große Blöcke), `LinkAudioReceiveModule`,
`StepSequencerModule` (bis auf V1). Die nicht-atomaren Struktur-Member
(`LooperPatchOutModule::specs`, `LooperPatchInModule::totalChannels`,
`LinkAudioSendModule::slots`) sind durch den Materialisierungs-Kontrakt
eingefroren: `applyOutputSpecs`/`applySendConfig` laufen [MT, VOR
`prepareForGraph`], Struktur-Änderungen re-materialisieren den Node
gefadet (Header-Doku + `GraphManager.cpp:1763–1766`) — kein Race.

**Zeitbasen-Sonderfall** — `CallbackTimingMonitor.h:29–32/59–68`:
`getHighResolutionTicks` (QPC) im Callback ist eine **dokumentierte,
begründete Ausnahme** zu §3.1 (lock-/allocation-frei, ausschließlich
Diagnose, nie Rendering-Zeitbasis). Alle übrigen
`getMillisecondCounter`/`chrono`-Treffer der Codebasis liegen auf
MT/Netzwerk/Writer-Threads (LooperTrashCan, LiveSpectrumTap-UI-Seite,
CaptureWriter, TouchLiveClient, UI).

**Querschnitt:**
- **Locks:** sämtliche `CriticalSection`/`ScopedLock`-Vorkommen
  (OscController-Registry [Netzwerk↔MT, „Audio NIE beteiligt"],
  CaptureWriter, CaptureService-reportLock, TouchLiveClient,
  BrowserFileScanner) sind vom Audio-Thread unerreichbar.
- **Strings/Logging:** `Logger::writeToLog` nur im MT-Graph-Swap
  (`GraphManager.cpp:1836/1952`) und `Main.cpp:73`; kein `DBG`, keine
  String-Ops in Audio-Pfaden.
- **ValueTree:** kein einziger ValueTree-Zugriff aus einem Audio-Pfad
  (§6-konform); Module binden ausschließlich über
  `getParameterTarget`-Atomics.
- **Zufall:** kein `rand()`, kein `mt19937`, kein `<random>`-Include im
  gesamten `Source/`-Baum (nur Kommentare/Doku); Ausnahme s. V1.
- **SPSC-Disziplin:** `SpscQueue` mit `static_assert
  (trivially_copyable)`, alignas(64)-Indizes; `ParameterUpdate` ist POD;
  `juce::AbstractFifo` kommt nirgends vor.
- **Indirekte Allokationen:** `std::function`-Member
  (`CaptureService.h:243/295`, LinkClock-Receive-Callback) werden nur
  auf MT/Link-Thread gerufen; `juce::dsp`-FFT in `LooperWaveformTap`
  mit Ctor-Warmup gegen Lazy-Init (RT-Audit-Test laut Kommentar);
  kein Modul befüllt `MidiBuffer` im Audio-Pfad.
- **RT-Audit-Infrastruktur:** `Util/RtAllocationGuard` ersetzt in
  Dev-Builds global `operator new/delete` und schlägt in
  `ScopedRealtimeSection`-Abschnitten an — die Engine-Bausteine des
  EngineProcessor-Callbacks sind damit mechanisch abgedeckt.

---

## 5. Empfohlene Folgeaufträge (priorisiert nach Schadenspotenzial)

1. **RT-Audit-Lücke Graph-Rendering schließen (hoch).**
   `graph.processBlock()` (`EngineProcessor.cpp:1096`) — also das
   Rendering ALLER Module inkl. der 114 Airwindows-Ports — läuft als
   einziger Callback-Abschnitt OHNE `rt::ScopedRealtimeSection`. Eine
   Section um den Graph-Aufruf (Dev-Builds) würde jede künftige
   Modul-Allokation mechanisch fangen statt per Code-Review.
2. **Materialisierungs-Kontrakt absichern (mittel).**
   `LooperPatchOutModule::applyOutputSpecs`,
   `LooperPatchInModule::applySendConfig`,
   `LinkAudioSendModule::applySendConfig` mutieren nicht-atomare
   Struktur-Member, die der Audio-Thread liest — heute safe (nur vor
   `prepareForGraph`), aber ohne mechanische Absicherung. Eine
   jassert-Guard („Node nicht im Graph") macht den Kontrakt bruchfest.
3. **V1 auflösen (mittel).** StepSequencer auf den §3.1-inline-LCG
   umstellen (IStochastic-Muster, Verhalten bleibt deterministisch pro
   Seed) ODER §3.1 um eine dokumentierte `juce::Random`-Ausnahme
   ergänzen — eine der beiden Seiten muss sich bewegen, sonst erodiert
   die Regel.
4. **V2 auflösen (niedrig).** Thread-Ownership-Doku von
   `AirwindowsPlugin::setParameter` korrigieren („jeder Thread,
   atomic-relaxed") oder den Chassis-Pfad auf direkte
   Snapshot-Übergabe umbauen — reine Doku-/API-Hygiene.
5. **Audit-Erwartung aktualisieren (niedrig).** Künftige
   RT-Audit-Aufträge sollten `Source/Interfaces/` (Kontrakt-Header) und
   `Source/TouchLive/` (LiveSpectrumTap-Audio-Producer) in der
   Erwartungsliste führen, damit die Abweichung nicht jedes Mal als
   Sonderfall geprüft werden muss.
