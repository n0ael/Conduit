---
paths:
  - "Source/Core/GridVoiceEngine.*"
  - "Source/Core/GridPanelSettings.*"
  - "Source/Core/PadGridLayout.*"
  - "Source/Core/RingTouchModel.*"
  - "Source/Core/ExpressionAxis.*"
  - "Source/Core/MpeEncoder.*"
  - "Source/Core/MpeMidiSink.*"
  - "Source/Core/VoiceAllocator.*"
  - "Source/Core/GridPhysics.*"
  - "Source/Core/GridSessionStore.*"
  - "Source/Core/GridSensitivity.h"
  - "Source/Core/ChordMemory.*"
  - "Source/Core/CcControlModel.*"
  - "Source/UI/Grid*"
  - "Source/UI/MpeShapingView.*"
  - "Source/UI/LockToggle.*"
  - "Source/UI/CcControlLayer.*"
  - "Source/UI/ChordMemoryStrip.*"
  - "Tests/Core/GridVoiceEngineTests.cpp"
  - "Tests/Core/PadGridLayoutTests.cpp"
  - "Tests/Core/RingTouchModelTests.cpp"
  - "Tests/Core/ExpressionAxisTests.cpp"
  - "Tests/Core/MpeEncoderTests.cpp"
  - "Tests/Core/MpeMidiSinkTests.cpp"
  - "Tests/Core/VoiceAllocatorTests.cpp"
  - "Tests/**/Grid*"
  - "Tests/**/ChordMemory*"
  - "Tests/**/CcControl*"
  - "docs/Grid.md"
---

# Rule: grid — Grid-Touch-Controller Ω (CLAUDE.md 10.0, ADR 002/003)

**Pflichtlektüre: docs/Grid.md** (Spezifikation + Meilensteinleiter).

- Kette Quelle → Voice-Modell → Sink; MPE-Zuteilung IN Conduit
  (`VoiceAllocator` + `MpeEncoder`).
- `GridVoiceEngine` ist Engine-Level, Message-Thread (ITouchMacro-Kette,
  CLAUDE.md 4.2) — kein Audio-Thread, kein Graph.
- Touch-Semantik: 1. Finger = Sonne (Note + Pitch-Bend X + Ausdruck Y);
  2. Finger im Orbit = Mond (Ring — Radius → Slide, KEINE neue Note).
- Achsen klemmen erst am Ausgang (pitchBendAxis/Encoder), nicht in der
  Geometrie (ADR 003).
