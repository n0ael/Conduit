---
paths:
  - "Source/Modules/ProcessorModule.*"
  - "Source/Modules/ChassisSchema.*"
  - "Source/Modules/Airwindows*"
  - "Source/DSP/**"
  - "Source/UI/FxModulePanel.*"
  - "Source/UI/GainFaderMeter.*"
  - "Tests/Core/ProcessorChassisTests.cpp"
  - "Tests/Core/AirwindowsModuleTests.cpp"
  - "docs/FxChassis.md"
---

# Rule: fx-chassis — FX-Chassis-Standard (CLAUDE.md 4.6)

**Pflichtlektüre vor jeder FX-Arbeit: docs/FxChassis.md** (Spezifikation:
Gains, Meter, Link-Send-Tap, CV-Richtungsmodell, Control-Linking,
Schema-/Migrations-Regeln, FxModulePanel).

Verbindlich für ALLE FX-Module:

- Jedes Audio-FX-Modul erbt `ProcessorModule` und implementiert NUR
  `prepareCore()`/`processCore()` (Stereo-Sicht, Kanäle 0..1);
  `prepareToPlay`/`processBlock`/`appendParametersTo`/
  `getParameterTarget` sind final — NIE überschreiben.
- Kanal-Layout FEST: Audio 0..1, CV 2..N
  (`ChassisSchema::cvChannelForParam`); Parameter-`role`:
  "dsp" | "chassis" | "cvAmount".
- Chassis-Bestandteile NIE nachbauen — sie leben zentral im Chassis.
- Airwindows-Ports: kein rand()/Heap im Prozesspfad (CLAUDE.md 3.1),
  deterministische Seeds; Port-Historie in Source/DSP/Airwindows/PORTING_NOTES.md.
