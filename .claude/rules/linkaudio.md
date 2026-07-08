---
paths:
  - "Source/Core/LinkClock.*"
  - "Source/Core/LinkSendTaps.*"
  - "Source/Core/LinkReceiveStream.*"
  - "Source/Core/InputLinkSend.*"
  - "Source/Core/SourceNameResolver.*"
  - "Source/Modules/LinkAudio*"
  - "Source/UI/LinkAudio*"
  - "Source/UI/LinkSend*"
  - "Source/UI/LinkMenuPanel.*"
  - "Source/UI/InputSendButton.*"
  - "Tests/Core/LinkAudioSendTests.cpp"
  - "Tests/Core/LinkAudioReceiveTests.cpp"
  - "Tests/Core/LinkSendTapsTests.cpp"
  - "Tests/Core/LinkReceiveStreamTests.cpp"
  - "Tests/Core/InputLinkSendTests.cpp"
  - "Tests/UI/LinkAudioSendPanelTests.cpp"
  - "Tests/UI/LinkAudioReceivePanelTests.cpp"
  - "docs/LinkAudio.md"
---

# Rule: linkaudio — Link Audio (CLAUDE.md 7.2)

**Pflichtlektüre vor jeder LinkAudio-Arbeit: docs/LinkAudio.md**
(Send-Mechanik, Empfangs-Spezifikation, Teardown-Race, Lektionen).

- **LinkAudio ERSETZT Link** — beide Klassen nie parallel instanziieren;
  `LinkClock`-Pimpl hält die einzige `ableton::LinkAudio`-Instanz für
  Timing UND Audio.
- IWYU: `<ableton/LinkAudio.hpp>` in JEDER Compilation Unit, die
  LinkAudio-Typen berührt — die Link-Header sind nicht selbsttragend.
- Format interleaved int16, Float→Int16 NUR mit TPDF-Dither (LCG);
  Sink-Größe in SAMPLES — Frames und Samples nie mischen.
- Module/Objekte, die einen `LinkClock*` halten, halten ihn IMMER als
  `juce::WeakReference<LinkClock>` (Teardown: die Clock kann in Test-Rigs/
  Shutdown vor dem Halter sterben — ASan-Fund 08.07.2026).
- `availableChannels()` listet NUR Peer-Announcements — eigene Sinks nie;
  ohne Peer ist die Liste leer. Peer-ChannelKeys sind session-transient:
  discoverbar, NIE serialisieren (CLAUDE.md 6) — persistiert wird der
  Kanal-Wunsch als Namen (targetPeer/targetChannel).
- Kein finales `enableLinkAudio(false)` unmittelbar vor der
  LinkClock-Destruktion erzwingen — der Destruktor-Pfad deaktiviert
  selbst und racefrei (deferred Disable, SDK-Teardown-Race).
- Send + Clock + Receive implementiert (Receive: beat-aligntes
  Latenzfenster `latency_ms`, LinkReceiveStream).
