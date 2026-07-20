# Grid-PlayTargets — M0-Inventur (Read-only)
> Durchgeführt: 20.07.2026 · Branch: master · Plan + Entscheidungen
> E1–E7: **[GridPlayTargets-Masterplan.md](GridPlayTargets-Masterplan.md)**
> (Multi-Ziel-MPE, M0–M6, Option B „MIDI als Kabeltyp" gated auf die
> Mono/Stereo-Entscheidung). Dieses Dossier ist die Referenz-Inventur
> für alle Folgemeilensteine M1–M6.
>
> Pflichtlektüre der Inventur: docs/Grid.md, docs/MidiRig.md, Rules
> `grid`/`midirig`/`ui-design`, ADR 002/003/006.

---

## 1. Pipeline-Karte (Touch → MIDI)

```
GridKeyboardComponent (Touch, Message Thread)
  → GridVoiceEngine            Source/Core/GridVoiceEngine.h — Engine-Level,
                               Message Thread, KEINE Atomics/Queues
  → IVoiceSink                 Source/Interfaces/IVoiceSink.h — Slot-indizierte
                               Voice-Events (voiceStart/Stop/PitchBend/Pressure/Slide)
  → MpeMidiSink                Source/Core/MpeMidiSink.h — MPE-MIDI 1.0 via MpeEncoder
  → IMidiOutputTarget          Source/Interfaces/IMidiOutputTarget.h — eine Methode
                               send(const juce::MidiMessage&)
  → MidiPortHub-Rollen-Fassade gridOutputTarget() (MidiPortHub.h:170) — löst die
                               Grid-Output-Rolle bei JEDEM send live auf
  → MidiDeviceTarget           sendMessageNow, Message Thread
```

Wiring/Besitz: `EngineProcessor.h:595-596` —
`MpeMidiSink mpeMidiSink { midiPortHub.gridOutputTarget() }` und
`GridVoiceEngine gridVoiceEngine { mpeMidiSink }`. Die Grid-Schicht kennt
weder MPE-Spezifika noch Ports; `GridKeyboardComponent` erhält nur
`GridVoiceEngine&`.

**Threading:** Der gesamte Ausgabepfad läuft synchron auf dem Message
Thread (kein Audio-Thread, keine SPSC-Queue — E7-Invariante MidiRig).
SPSC-Queues existieren nur eingangsseitig (MidiControlInput/MidiNoteInput/
MidiPortHub-InputConnections: MIDI-System-Thread → SpscQueue → 60-Hz-Drain).

## 2. B-Readiness-Befund: Die Sink-Naht EXISTIERT bereits

Die im Masterplan geforderte „IMidiSink"-Naht ist implementiert (ADR 003):
`IVoiceSink` (Voice-Ebene) + `IMidiOutputTarget` (Message-Ebene) + die
live-auflösenden Hub-Rollen-Fassaden (`GridOutputFacade`,
`MidiPortHub.h:245-256`). **PlayTarget-M1 dockt hier an — kein neues
Interface nötig.** Ein PlayTarget = (MpeMidiSink-Instanz + eigener
Allocator) → eigenes `IMidiOutputTarget`.

**MPE-Allocator heute:** `VoiceAllocator` (15 Slots, Stealing = ältester,
`kMaxVoices = 15`) + `MpeEncoder` (Lower Zone FEST, Member-Kanäle ab 2,
Master 1, `memberChannelBase` Config-fix — keine dynamische
Zonen-Konfiguration, keine Upper Zone). Es gibt genau EINEN globalen
`MpeMidiSink`, gebunden an die Grid-Output-Rolle der Registry.
**Per-Target-MPE = Instanziierung von Sink + Allocator pro Ziel** (die
Klassen sind dafür bereits geeignet; testbar via `FakeMidiTarget`).

## 3. Registry-Andockpunkt (M2: preferredGridMode, Farbe, Voice-Zahl)

`struct RigDevice` (`MidiRigSettings.h:21-44`) trägt heute
`id (Uuid), label, kind, midiOutName, midiInName, midiChannel,
controllerProfileName, takeoverMode`; Rollen (`gridOutputDeviceId`,
`gridControllerDeviceId`, `liveRemoteDeviceId`) leben auf Registry-Ebene.
**Muster für neue Felder:** Feld in RigDevice + Setter (Vorbild
`setTakeoverMode`/`setControllerProfileName`) + ValueTree-Property +
Combo in `MidiRigSettingsComponent`. Profil-inhärente Daten dagegen als
neue header-getriebene CSV-Spalte (Rule midirig: „neue Fähigkeit nur über
Daten, nie Gerätecode"). Voice-Zahl/Port-Zuordnung stehen NICHT im CSV —
Voice-Zahl lebt in `MpeEncoder::Config`, Ports in der RigDevice-Registry.

## 4. AbletonOSC-Gap (E2: Routing-Lookup ist Neuland)

Der bestehende OSC-Verkehr (TouchLive-Subsystem, `TouchLiveClient`,
`OscController`, `OscSendService`) ist **Command-only**: `/live/*/set/…`,
`/live/clip_slot/fire`, `/live/browser/load`,
`/live/song/set/midi_input_focus`. **Es existiert KEIN Code, der
Tracknamen oder `input_routing`-Properties LIEST** (Grep über
`Source/TouchLive/**` auf `input_routing`, `track/get`, `get/name`:
0 Treffer — Tracknamen kommen über das LiveSetModel-Sync, nicht über
Get-Abfragen). Der für M2 geplante Routing-Lookup (Track hört auf
`C-<Gerät>` → Mapping) braucht daher den AbletonOSC-Gap-Check
(Verfügbarkeit von `track.input_routing_type` in der eingesetzten
Version), BEVOR die Fork-Frage entschieden wird.

## 5. Bug-Diagnose Slide-Reacquisition (M0-Fix — BEHOBEN 20.07.2026)

Symptom: gehaltene (Drone-)Note erneut berühren → Pitch reagiert,
Slide/Orbit tot. Ursache (verifiziert): der Drone-Grab-Pfad
(`GridKeyboardComponent.cpp`, touchDown/touchMove-Grab-Zweige) sendete
nur Bend + Pressure — `engine.setSlide` fehlte im gesamten Grab-Zweig,
UND das `RingTouchModel` vergisst die Sonne beim Drone-Start
(`onUp` entfernt die Primary) — kein Finger konnte den eingefrorenen
Mond wieder aufgreifen (bei lebenden Sonnen leistet das `onDown`).
Gelatchte Sonnen waren gar nicht re-touchbar.

Fix (Block M2, gleiche Session): Mond-Reakquisition für fingerlose
Sonnen + voll grabbare Latch-Sonnen + Release-All-Slot-Löschen —
Spezifikation in docs/Grid.md (Block M/Akkord-Speicher), Regression in
`Tests/UI/GridKeyboardTests.cpp` (`[drone]`/`[latch]`).

## 6. Audit-Status (Risikoversicherung des Umbaus)

AUDIT-01 (RT-Safety), AUDIT-02 (Threading), AUDIT-04 (Lifetime) — alle
am 20.07.2026 auf master (b1f998c) durchgeführt: **0 harte Verstöße**,
nur dokumentierte Verdachtsfälle (docs/audit/). Die Touch→MIDI-Pipeline
ist damit auditiert; der PlayTarget-Umbau baut auf geprüftem Boden.

## 7. Konsequenzen für M1 (Kurzliste)

1. PlayTarget-Abstraktion = benannter `IMidiOutputTarget` + eigene
   `MpeMidiSink`/`VoiceAllocator`-Instanz; Registry liefert die Ziele.
2. Die noteOn-Zielbindung (Route-Binding, E1: noteOn-immutabel) gehört in
   die Schicht ZWISCHEN GridKeyboardComponent und den Sinks — die
   GridVoiceEngine ist der natürliche Ort (sie besitzt die Voice-Slots
   und die Achsen; ein Ziel-Index pro Voice-Slot genügt).
3. Kein Audio-Thread beteiligt — der „lock-freie Ziel-Lookup" aus dem
   Masterplan relativiert sich: alles Message Thread, ein einfacher
   Zielindex reicht (keine Atomics nötig, Rule grid/E7).
4. Virtuelle Ports (E6): `MidiDeviceTarget` öffnet nur existierende
   Ports; WMS-Spike (app-erzeugte `C-<Track>`-Ports) bleibt eigener
   Strang, loopMIDI-Pfad ist sofort gangbar.
