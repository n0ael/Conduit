---
paths:
  - "Source/UI/TransportBar.*"
  - "Source/Core/Metronome.*"
  - "Source/Core/TransportSettings.*"
  - "Source/Core/LaunchQuantization.*"
  - "Source/Core/TapTempo.h"
  - "Tests/Core/TransportLinkTests.cpp"
  - "Tests/Core/MetronomeTests.cpp"
  - "Tests/Core/LaunchQuantizationTests.cpp"
  - "Tests/Core/ClockTests.cpp"
  - "docs/Transport.md"
---

# Rule: transport — TransportBar, Metronom, Launch-Quantisierung (CLAUDE.md 10.0/4.5)

**Pflichtlektüre: docs/Transport.md** (TransportBar-Bestandteile,
Tap-Tempo-Modell, Metronom).

- TransportBar ersetzt die Modul-Button-Toolbar komplett; Transport-/
  Link-Zustand in `TransportSettings` (App-Zustand, Muster MeterSettings);
  der EngineProcessor speist LinkClock (Start/Stop-Sync, Clock-Offset
  ±100 ms als Beat-Lese-Versatz) und Metronom (Enable, Anker).
- Metronom = allocation-freier Click NACH dem GraphFader, Beat-Grenzen
  sample-genau; bewusst kein isPlaying-Gate (Conduit läuft frei).
- Launch-Quantisierung (4.5, kanonisches Muster für IClockSlave-Module):
  Grid-Enum zentral und app-weit (`Source/Core/LaunchQuantization.h`,
  None/8/4/2/1 Bar, 1/2 … 1/32). UI/OSC setzt atomare Pending-Flags;
  der Audio-Thread erkennt die Grid-Überquerung pro Block sample-genau
  (`floor(beat/qBeats) > floor(prevBeat/qBeats)`), führt die Aktion aus
  und konsumiert das Flag per `exchange(false, acq_rel)`; qBeats == 0 →
  sofort am Blockanfang.
- Link-Tests: `requestIsPlaying` NUR mit deaktiviertem Start/Stop-Sync
  testen (sonst startet die Ableton-Session des Users); Tempo-Assertions
  poll-basiert (Link merged Commits async).
