### ADR 007: SysEx-Empfang für den Hardware-Preset-Browser (Amendment zu 006 E6)

Status: Akzeptiert — Juli 2026 (User-Entscheidung 17.07.2026)

Kontext:
ADR 006 E6 begrenzt SysEx auf Sende-Snippets — kein Parsing eingehender
SysEx, kein Feedback-Pfad. Der User-Wunsch „Presets eines Klangerzeugers
(DSI Mopho) per Push-Button laden, mit NAMEN auf den Buttons" erfordert
das Auslesen der Programm-Dumps: der Name (16 ASCII-Zeichen) existiert
NUR im SysEx-Program-Dump des Geräts. Program Change selbst (das Laden)
bleibt der bestehende E5-Pfad und braucht kein SysEx.

Entscheidung:

E6 wird EINGESCHRÄNKT aufgehoben. Das MIDI-Rig darf eingehendes SysEx
empfangen und parsen, aber ausschließlich:

A1  Als Antwort auf einen von Conduit selbst gesendeten, explizit vom
    User angestoßenen Request (Preset-Scan-Dump-Requests, Universal
    Device Inquiry). Kein unaufgefordertes SysEx-Feedback, kein
    Dauer-Lauschen.

A2  Nur solange der Eingangsport dafür „armed" ist
    (`MidiPortHub::setSysExCaptureEnabled`, gesetzt vom Scanner für die
    Dauer der Sequenz). Im Normalbetrieb wird SysEx weiterhin am
    Producer verworfen — der Alltagspfad bleibt E6-konform.

A3  Transport bleibt E4/E7-konform: SysEx reist als Chunk-Strom
    (`midi::SysExChunk`, POD 64 Bytes, `SpscQueue<SysExChunk>{256}` PRO
    PORT) vom MIDI-System-Thread zum 60-Hz-Drain; Reassembly auf dem
    Message Thread. Der Producer pusht eine Nachricht NUR komplett
    (getNumFree-Vorabprüfung) — nie halbe Dumps; verworfene Nachrichten
    heilt der Scanner per Timeout-Re-Request. Nachrichten-Cap
    `midi::kMaxSysExBytes = 1024` (Mopho-Dump = 299 Wire-Bytes) hält
    Bulk-Fremdverkehr (Firmware-Updates) aus den Queues. Der
    Audio-Thread ist weiterhin NIE beteiligt.

A4  Protokollwissen lebt in reinen, hardwarefreien Funktions-Namespaces
    (`Source/Core/Sysex/DsiSysex` — Packed-MS-Bit-Codec, Dump-Request/
    -Parser, Inquiry, Name-Extraktion). KEINE CSV-Schema-Erweiterung;
    ein Dialekt-Interface erst, wenn ein dritter Hersteller dazukommt
    (DSI-Familie teilt das Protokoll, nur Device-ID-Bytes variieren).

Weiterhin VERBOTEN (unverändert aus E6):
- Patch-Editing über SysEx (Parameter-Schreiben via NRPN/CC, E1/E5).
- Checksummen-Protokolle (Roland u. ä.) — Geräte, die sie erfordern,
  sind out-of-scope.
- SysEx als Bindungs-/Trigger-QUELLE (MidiInBindings bleiben CC/NRPN/
  Note/PB/PC).
- `handlePartialSysexMessage` bleibt unbenutzt, solange kein realer
  Treiber Partial-Delivery zeigt (Erweiterungspunkt: der Reassembler
  ist offset-basiert und könnte Partials aufnehmen).

Konsequenzen:
+ Preset-Namen-Scan (HardwarePresetScanner/-Library, M9b) und künftige
  read-only-Dump-Features (z. B. Edit-Buffer-Anzeige) haben einen
  sauberen, eng umrissenen Empfangspfad.
+ Die Sende-Snippets aus dem ursprünglichen M9 (Hex + `{v}`) sind davon
  unabhängig und laufen als eigener Meilenstein (M10) weiter.
- Der Chunk-Transport ist bewusst NICHT für Dauerströme ausgelegt
  (256 Chunks ≈ 14 KB Fenster pro Drain) — wer mehr will, braucht ein
  neues ADR.
