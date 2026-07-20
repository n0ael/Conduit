# AUDIT-04 — Ownership-/Lifetime-Audit (Zombie-UI, Dangling)

> Read-only-Audit von Objekt-Ownership, Listener-Lebenszyklen und
> Delete-Pfaden gegen CLAUDE.md §2/§5/§6, docs/PatchEngine.md (5.1–5.7),
> Rules `patch-engine`/`linkaudio`/`node-editor`, docs/NodeEditor.md ·
> Durchgeführt: 20.07.2026 · Branch: master (b1f998c) ·
> Vorgänger: AUDIT-01/02/03

---

## 1. Zusammenfassung

Die Lifetime-Disziplin des Projekts ist außergewöhnlich gut: **Alle 56
Listener-Registrierungen haben ein deterministisches Gegenstück** (65
remove-Stellen — Destruktor plus, wo vorgesehen, Phase-1-Hook), **kein
einziges ungeschütztes `[this]` in `callAsync`/`callAfterDelay`**
(durchgehend `Component::SafePointer`/`WeakReference`), **kein
UI→Processor-Pointer** (bestätigt und vertieft aus AUDIT-03: der
sanktionierte Weg ist die refcount-basierte `NodeUiRegistry`), und der
**zweiphasige Delete-Pfad ist exakt wie in docs/PatchEngine.md 5.3
implementiert** — inklusive des Multipage-Falls „Node auf fremder Seite
ohne NodeComponent" (Refcount 0 → Phase 2 startet sofort, kein
Deadlock). Die LinkAudio-WeakReference-Pflicht ist bei allen
Clock-Haltern erfüllt — **mit einer Ausnahme: `GraphManager::linkClock`
ist ein roher Pointer** (V1, produktiv durch Deklarationsreihenfolge
sicher, formal gegen die Rule). Zweiter Befund (V2): zwei
Sequencer-Unterpanels erfüllen den Phase-1-Wortlaut („deregistriert
alle Listener") nur teilweise — UAF-frei, aber inkonsistent zum
Dossier. Ergebnis: **0 akute Dangling-/Zombie-Risiken, 2
Verdachtsfälle, mehrere positive Absicherungsmuster** (Registry-Gate,
Epoch-Handshakes, SafePointer-Konvention, Token-basierte
MIDI-Subscriptions).

---

## 2. Unpaarige/riskante Listener

**Inventur: 56 add- / 65 remove-Zeilen — vollständige paarweise
Zuordnung, keine unpaarige Registrierung gefunden.** Die Muster:

| Muster | Klassen | Bewertung |
|---|---|---|
| add im Ctor → remove im Destruktor | AudioDeviceController, GraphManager, OscController (+`cancelPendingUpdate`), OscSendService, MidiPortHub, LiveRemoteBridge, TouchLiveClient (+UdpRemoteTransport), LiveSpectrumTap, EngineProcessor (5 Quellen ↔ 5 removes :279–283), EngineEditor (5 ↔ 5 :537–541), CaptureSettingsComponent, CapturePanel, OscSettingsComponent, UiSettingsComponent, MidiRigSettingsComponent, GridSettingsView, PageOverviewComponent, TouchLivePage/-GridView/-MixerView/-DeviceView, StepGridDisplay, SequencerControlPanel | ✓ deterministisch |
| add im Ctor → remove in `stopUpdates()` (Phase 1) UND Destruktor | NodeComponent (:244/:258 + uiSettings/channelNames doppelt), FxModulePanel, GainFaderMeter, ParameterPanel, LinkAudioSendPanel, LooperPatchInPanel, LooperPatchOutPanel | ✓ dokumentiertes 5.3-Muster |
| add im Ctor → remove in `stopUpdates()`, Destruktor **delegiert** an stopUpdates | LinkAudioReceivePanel (`~…() { stopUpdates(); }`, :37–41) | ✓ (nur EIN remove-Ort, aber vom Destruktor mitgenutzt — geprüft, kein Loch) |
| bedingtes add (nullptr-Guard) → geguardetes remove | LinkAudioReceiveModule (WeakReference-Guard :79–87), NodeCanvas/NodeComponent (channelNames/uiSettings „nullptr in Tests") | ✓ |
| GridPage: rootState/liveSetState/midiRigSettings (:166/:363/:602) | removes :623–629 ✓ | ✓ |

**Token-basierte Subscriptions (MidiPortHub):** GridPage
(:624–627 Destruktor + :634–636 Re-Bind), LiveRemoteBridge (:86–88 +
:123–124), HardwarePresetScanner (:24/:164) — jede `subscribe*`-Stelle
hat deterministisches `unsubscribe`; die `[this]`-Callbacks leben im
langlebigeren Hub, der Guard ist die Token-Disziplin. ✓

---

## 3. UI→Processor-Verstöße (§5)

**Keine.** Vertiefung des AUDIT-03-Befunds:

- 0 Member/Referenzen vom Typ `EngineProcessor`/`ConduitModule`/
  `*Module` in Source/UI-Headern (Grep über alle `.h`).
- Die Modul-Header-Includes in UI (LooperPatchOutPanel, LinkAudio-
  Panels, NodeComponent.cpp …) ziehen ausschließlich Statics/Enums/
  `id::`-Identifier — keine Instanz-Bindung.
- Der sanktionierte Zugriffsweg ist die **NodeUiRegistry**
  (`NodeUiRegistry.h`): `acquire` einzig in `NodeComponent`-Ctor (:52),
  `release` einzig im Destruktor (:245, „gibt eine wartende Phase 2
  frei"); GraphManager startet Phase 2 nur bei `getRefCount == 0`
  (GraphManager.cpp:1649) und wird via `onNodeFullyReleased` erneut
  angestoßen.
- Rohe Service-Pointer in UI (`LevelMeter*`, `LooperWaveformTap*`,
  `ChannelNames*`, `InputLinkSend*`, `UiSettings*`,
  `LiveSpectrumTap*` — NodeCanvas.h:252–257, NodeComponent.h:248–253,
  LevelMeterBar, LooperWaveformStrip, TouchLive-Views) sind
  **non-owning auf EngineProcessor-Member, die jede UI überleben**
  (Deklarationsreihenfolge-Kontrakt, „nullptr in Tests" dokumentiert) —
  kein §5-Verstoß, da keine Processor-/Modul-Objekte.

---

## 4. Async-Capture-Risiken ([this] ohne Guard)

**0 ungeschützte Treffer.** Grep über `callAsync ([this` und
`callAfterDelay (N, [this` ist leer; alle async-Hops sind geguarded:

| Ort | Guard |
|---|---|
| NodeComponent.cpp:322 (Phase-2-Hop), :1182 (Delete-Disarm) | `Component::SafePointer` |
| FxModulePanel.cpp:462, EngineEditor.cpp:672, GridPage.cpp:1486, OscSettingsComponent.cpp:55, LooperPage.cpp:52, MpeShapingView.cpp:350, LinkAudioReceivePanel.cpp:117 (Menü-Callback) | `Component::SafePointer` |
| LinkClock.cpp:120 (ChannelsChanged → MT) | `WeakReference<LinkClock>` |
| PushTiles.cpp:366 (Editor-Entsorgung) | `shared_ptr`-Capture, kein `this` |
| Browser (SearchIndex/FileScanner) | injizierbarer Dispatcher + Generation-Zähler (Header-Doku) |
| TouchKeyboard | `SafePointer<TextEditor>` (Ziel „darf jederzeit sterben") |

Die 385 synchronen `= [this]`-Lambdas (Button-`onClick`,
`onValueChange` …) binden an gleichlebige Member-Widgets — kein
async-Fenster. Member-`juce::Timer` stoppen im JUCE-Destruktor
automatisch. Die einzigen `[this]`-Lambdas in **langlebigeren**
Fremdobjekten sind die MidiPortHub-Subscriptions (Abschnitt 2,
Token-Disziplin ✓) und `CaptureService`-/`OscController`-Hooks, die
ausschließlich vom **Owner** (EngineProcessor) gesetzt werden ✓.

---

## 5. Delete-Pfad-Befunde inkl. Multipage

**Kette verifiziert (NodeComponent.cpp:249–339 gegen PatchEngine 5.3):**
Phase 1 `beginTeardown()` — idempotent (`tearingDown`), Tree-Listener
sofort weg, Timer/Interaktion aus, `stopUpdates()` an alle 8
Kind-Panel-Typen + Send-Buttons + MeterBars; danach
`teardownVBlank` → einmaliger Dispatch (`teardownNotified`) →
`callAsync` mit SafePointer → `completeTeardownNow` →
`onTeardownFinished` zerstört. Registry-Release erst im Destruktor —
GraphManager-Phase-2 (`pendingDeletes`, :1636–1674) prüft
`getRefCount > 0` und wird von `onNodeFullyReleased` reaktiviert.
`OscController` cached moduleId in Phase 1 (`pendingDeletes[nodeUuid] =
…moduleId`, :1236). Undo-Restauration mit `nodeState == Deleting` wird
behandelt (:1905, Test :147). **Vollständig, kein Loch.**

**Multipage (jüngster Code, 18.07.2026):**

- **Node auf fremder Seite ohne NodeComponent:** nie `acquire` →
  Refcount 0 → Phase 2 startet ungehindert. Kein Deadlock, kein
  Zombie. ✓
- **Seitenwechsel = reines `setProperty (pageUuid)`**
  (PageManager.cpp:145–148 mit ADR-008-Kommentar); `removeChild` nur
  beim echten Seiten-Löschen (:172) — Delete-Pfad/OSC feuern nicht
  fälschlich. ✓
- **Seitenwechsel-Rebuild:** alte NodeComponents werden destruiert —
  Destruktor macht removeListener + release; ein gerade
  Deleting-Node der alten Seite verliert seine UI-Referenz dadurch
  ebenfalls sauber. ✓
- **Delete-Armierung** (Doppel-Tap, 3 s): Disarm über
  `callAfterDelay` + SafePointer (NodeComponent.h:338, dokumentierte
  Lektion NodeEditor.md §5). ✓
- **Cross-Page-Kabel:** leben ausschließlich im Tree/Graph
  (View-agnostisch); Node-Delete entfernt sie über den normalen
  GraphManager-Pfad (`removeNode` entfernt Graph-Kabel,
  Tree-Connections in der Delete-Transaktion) — keine UI-Halter,
  keine Lifetime-Inkonsistenz. ✓
- **PageOverviewComponent:** rootState-Listener Ctor/Destruktor
  (:15/:30), VBlank-Miniaturen-Rebuild als unique_ptr-Member. ✓

**Verdachtsfälle:**

### V1 — `GraphManager::linkClock` als roher Pointer (Rule linkaudio)

`GraphManager.h:528` (`LinkClock* linkClock = nullptr`), gesetzt via
`setLinkClock` (:1166), genutzt bei der Materialisierung (:1715). Die
Rule verlangt: Halter eines `LinkClock*` „IMMER als
`juce::WeakReference`" (Teardown-/Test-Rig-Begründung, ASan-Fund
08.07.2026). Alle anderen Halter sind konform (`LinkSendTaps.h:171`,
`LinkAudioReceiveModule.h:162`, `LiveSpectrumTap.h:110`). Entschärfung:
produktiv ist `linkClock` (EngineProcessor.h:432) VOR `graphManager`
(:570) deklariert — der Manager stirbt zuerst; `GraphManagerTests`
setzen nie eine Clock (Grep leer). **Kein akutes Risiko, formale
Rule-Abweichung** — beim nächsten Test-Rig mit Clock würde die Falle
scharf.

### V2 — Phase-1-Wortlaut bei Sequencer-Unterpanels

PatchEngine 5.3: UI „deregistriert alle Listener" in Phase 1.
`StepGridDisplay::stopUpdates()` stoppt nur den FramePacer (:35–38),
der Tree-Listener bleibt bis zum Destruktor; `SequencerControlPanel`
hat gar keinen stopUpdates-Hook (beginTeardown ruft nur
`setEnabled (false)`, NodeComponent.cpp:278–279). **UAF-frei** (der
Subtree lebt bis Phase 2, die Destruktoren deregistrieren, das
Registry-Gate schützt), aber während `Deleting` verarbeiten beide
weiter Tree-Callbacks — inkonsistent zu den übrigen 7 Panels und zum
Dossier-Wortlaut.

---

## 6. Abgleich mit ASan-Abdeckung (§13.4)

ASan läuft lokal als Preset (`asan`, MSVC) und in CI (`asan-linux`,
jeder Push) über `ConduitTests`.

**Durch ASan-gedeckte Testpfade abgedeckt:**

| Befund/Pfad | Testdatei |
|---|---|
| Zweiphasiges Delete komplett (Phase 1 → Deleting, Phase 2 + Kabel, Undo-mit-Deleting, Registry-Gate `uiRegistry.release`) | `GraphManagerTests.cpp` (:338 ff., :403), `NodeCanvasTests.cpp` (:124/:143/:199) |
| OSC-Deregistrierung Phase 1 | `OscControllerTests.cpp` |
| Panel-stopUpdates/Zombie-Schutz | `FxModulePanelTests.cpp` |
| LinkClock-Teardown-Rennen (WeakReference-Halter) | `LinkAudioReceiveTests`, `LinkSendTapsTests`, `LinkReceiveStreamTests` (historischer ASan-Fund 08.07.2026 → Regression gedeckt) |
| Looper-Retire/Graveyard (kein free im Callback) | `LooperBankTests`, `LooperTrashTests` |

**NICHT ASan-gedeckt (kein Testpfad):**

1. **Der echte VBlank→callAsync-Phase-2-Hop:** headless feuert VBlank
   nicht — Tests rufen `completeTeardownNow()` direkt
   (NodeCanvasTests:36, dokumentiert). Der SafePointer-Guard des Hops
   (NodeComponent.cpp:322) ist damit nie unter ASan gelaufen.
2. **V1:** `GraphManager::linkClock` — kein Test setzt je eine Clock
   am GraphManager; die Rohhalter-Falle ist unbeobachtet.
3. **V2-Restfenster:** Tree-Callbacks in StepGridDisplay/
   SequencerControlPanel während `Deleting` — funktional getestet nur
   implizit.
4. **LiveSpectrumTap-Teardown** (WeakReference + Producer-Stopp) —
   `LiveSpectrumTapTests` single-threaded, kein Destruktions-Rennen
   (deckt sich mit AUDIT-02-Befund zur TSan-Lücke).
5. **MidiPortHub-Port-Close** während laufender MIDI-Flut
   (Handle-Destruktor stoppt Producer vor Queue-Tod — Member-
   Reihenfolge-Kontrakt, MidiPortHub.h:192–193) — nur single-threaded
   getestet.

---

## 7. Empfohlene Folgeaufträge

1. **(mittel)** V1 beheben: `GraphManager::linkClock` auf
   `juce::WeakReference<LinkClock>` umstellen (mechanisch, 3
   Verwendungsstellen) — schließt die Rule-Abweichung und macht
   künftige Test-Rigs mit Clock gefahrlos.
2. **(mittel)** V2 vereinheitlichen: `StepGridDisplay::stopUpdates`
   um `nodeTree.removeListener (this)` ergänzen;
   `SequencerControlPanel` einen stopUpdates-Hook geben und in
   `beginTeardown` einhängen — ODER PatchEngine 5.3 präzisieren
   („Rendering stoppen; Listener-Deregistrierung spätestens im
   Destruktor, Registry-Gate schützt Phase 2").
3. **(mittel)** ASan-Lücke 1 schließen: einen Testpfad für den
   VBlank-Hop schaffen (z. B. `beginTeardown` + manuelles Feuern des
   VBlank-Callbacks über einen Test-Seam), damit der
   SafePointer-Guard real ausgeführt wird.
4. **(niedrig)** LiveSpectrumTap-/MidiPortHub-Teardown-Tests mit
   echten Threads — deckungsgleich mit AUDIT-02-Folgeaufträgen 1/2
   (ein gemeinsamer Meilenstein „Threaded-Teardown-Tests" bündelt
   AUDIT-02 F1/F2/F4 und AUDIT-04 F3/F4).
5. **(niedrig)** Die stillen Lebensdauer-Kontrakte („Service-Pointer
   in UI sind EngineProcessor-Member und überleben jede UI";
   „GraphManager-Service-Pointer folgen der Deklarationsreihenfolge")
   als je einen Satz in Rule `patch-engine`/`ui-design` aufnehmen —
   sie sind heute nur in Member-Kommentaren dokumentiert.
