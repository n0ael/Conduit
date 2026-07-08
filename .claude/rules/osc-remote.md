---
paths:
  - "Source/Core/OscController.*"
  - "Source/Core/OscSendService.*"
  - "Source/Core/OscAddress.*"
  - "Source/Core/OscSendSettings.*"
  - "Source/Core/RemoteModuleBinder.*"
  - "Source/UI/OscSettingsComponent.*"
  - "Tools/Max/**"
  - "Tests/Core/OscControllerTests.cpp"
  - "Tests/Core/OscSendServiceTests.cpp"
  - "Tests/Core/RemoteModuleBinderTests.cpp"
  - "docs/OscSend.md"
  - "docs/M4LAnnounce.md"
---

# Rule: osc-remote — OSC-Integration & M4L-Announce (CLAUDE.md 7.1/7.3/7.4)

**Pflichtlektüre: docs/OscSend.md (Send-Pfad) und docs/M4LAnnounce.md
(Remote-Module)** — je nachdem, welches Teilsystem berührt wird.

Auto-Registration (7.1):
- `OscController` lauscht global als `ValueTree::Listener` auf den Root-Tree;
  `valueTreeChildAdded` → liest `type` + `moduleId` → registriert Adressen.
- `moduleId` wird bei `nodeState → Deleting` gecacht (Phase 1);
  Deregistrierung in Phase 1, NICHT erst in `valueTreeChildRemoved`.
- DSP-Module wissen nichts von OSC — Single Responsibility.

OSC-Send (7.3):
- Snapshot-Diff via `juce::Timer` @ 30 Hz auf dem Message Thread,
  Cache-Key `(nodeUuid, paramId)`; der Audio Thread ist NIE beteiligt.
- Default-Port 9001, NICHT 9000 (Loopback-Schutz gegen den eigenen Empfang).
- Float-Diff IMMER beidseitig über `float` vergleichen (`juce::exactlyEqual`),
  sonst Dauersende-Schleife (var speichert double).

Max4Live-Announce (7.4):
- `remoteId` = dokumentierte Ausnahme zur Laufzeit-ID-Regel (CLAUDE.md 6):
  in Live-Set UND Patch persistent, hartes Format `[A-Za-z0-9_-]`, max. 64
  Zeichen.
