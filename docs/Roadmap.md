# Conduit — Feature-Roadmap

> Frei fortschreibbare Ideen-/Feature-Liste — Scope-Grenzen definiert
> CLAUDE.md §12; Versionierung unabhängig von CLAUDE.md.

Erledigte v2.0-Features stehen nicht mehr hier — sie sind in den Dossiers
dokumentiert: Link Audio Send, Transport-Header, FX-Chassis, Looper inkl.
Vollausbau, OSC-Send, M4L-Announce (+ Max-Testdevice ConduitLFO), Grid M1.

| Feature | Version | Notiz |
|---|---|---|
| Link Audio Receive | v2.x | implementiert 08.07.2026 (docs/LinkAudio.md) — Feldtest iPad+Ableton kabelgebunden bestanden 07/2026 |
| I/O-Konsolidierung (User-Idee 08.07.2026) | v2.x | audio_input/audio_output starten stereo, „+" fügt Hardware- ODER Link-Kanäle hinzu (ein Modul für alle Ins, eins für alle Outs); InputSendButtons entfallen dann; Receive-/Send-Motoren bleiben die Basis |
| Gate, EQ, Compressor | v2.0 | ProcessorModule, ISidechain |
| CVTunerModule | v2.0 | AnalysisModule, CalibrationProfile |
| CLAP-Hosting | v2.x | PluginModule wraps AudioPluginInstance |
| IPolyphonic | v2.x | Interface vorbereitet, noch nicht implementiert |
| VST3-Hosting | v3.0+ | Steinberg-Lizenz, nach CLAP |
| Cardinal/VCV Integration | v3.0+ | Touch-native Modular UI |
| Expert-Sleepers-Encoder (ES-5/ES-4(0)/8CV/8GT) | v2.x | v1-Port vorhanden (EncoderEngines.hpp, MIT/VCV) — HardwareIOModule-Grundstein |
| Euclid-/Turing-Module | v2.x | v1-Engines als Referenz (Launch-Quant, parametrischer Swing, Scale-Quantize) |
| Mixer-Page | v2.x | ∥∥-Icon, Channel-Strips (Capture-Buttons wandern dorthin) |
| Grid-Page weitere Meilensteine | Ω-Icon | Meilensteinleiter: docs/Grid.md |
| Clip-Page (Fugue-Machine-Sequencer) | v2.x | ▷▭-Icon, immer aktiv, CV- UND MIDI-Ziele; Slot 2 vorerst an TouchLive abgegeben (09.07.2026) |
| TouchLive-Page (Ableton-Live-Remote) | v2.x | M1–M4 + M5/EQ-Eight erledigt (GRID/MIXER/DEVICE/BROWSER auf Slot 2, Meter, Fast-Path, bespoke EQ-Kurve), Meilensteinleiter: docs/TouchLive.md |
| Capture-Netzwerk-Share (Exports für entferntes Ableton) | v2.x | HTTP-Bereitstellung der Capture-Dateien |
| MIDI-Rig (Hardware-Mapping, NRPN/PC/SysEx) | v2.x | Meilensteinleiter: docs/MidiRig.md |
| Node-Page Multipage (Canvas-Seiten) | v2.x | M0–M4 umgesetzt 18.07.2026 (ADR 008/009, Rule `node-editor`, docs/NodeEditor.md); Feldtest 20.07.2026 bestanden nach zwei Fixes: Load-Roundtrip (PageManager::migrateAndRepair) und PageOverview-Callback-UAF (Stack-Kopie-Muster); Regressionen in PageLoadRoundtripTests. M5 Portal-Badges ZURÜCKGESTELLT (Bedarfsprüfung: lokale Outs pro Seite decken den Hauptfall; Cross-Page-Kabel wirken im Graph, sind nur unsichtbar) |
| GestureHelper-Spike (separater Prozess) | v2.x, nachrangig | Raw-Trackpad-Multitouch + Systemgesten-Umschaltung je OS; Muster Push-Shuttle (Prozess-Firewall für Private-APIs); NUR falls Modifier-Pfad in der Praxis nicht reicht |
| AUv3-Hosting (iOS/macOS) | v2.x | JUCE-nativ, lief im Erstversuch (iOS + macOS 17.07.2026); auf iOS einziges Plugin-Format; unabhängig von CLAP-Priorität |
| PageManager: seiteneffektfreier Lesepfad | v2.x | getActivePageUuid() mutiert als Getter den Tree (Wurzel Load-Bug 20.07.) |
| Callback-Selbstzerstörungs-Audit | v2.x | std::function-Inventur aus BUG-PAGENAV-Bericht abarbeiten (prioritär NodeComponent::onTeardownFinished — strukturgleich zum gefixten UAF); Stack-Kopie als Standard-Muster etablieren (Positiv-Referenz: DevPanel::closeDevPanelAsync) |
