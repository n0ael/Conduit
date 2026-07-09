# ConduitRemote

OSC Remote Script für Ableton Live 11/12 — Gegenstück zur TouchLive-Page in Conduit. Meilenstein **M1a** aus `TOUCHLIVE.md`: Transport, Tracks,
Mixer, Session (Clip-Grid) mit Push-Sync und Fast-Path.

## Warum nicht einfach AbletonOSC?

Alle verbreiteten OSC-Scripts (AbletonOSC, touchAble, …) lesen ihren UDP-Socket
nur im ~100-ms-Scheduler-Tick von Live — eingehende Fader-Werte werden mit
**10 Hz** angewendet, daher das bekannte Ruckeln. ConduitRemote hat einen
**Fast-Path**: ein eigener Empfangs-Thread wendet reine Wert-Schreibungen
(Volume/Pan/Send) **sofort** an; alles Strukturelle bleibt sicher im
Main-Thread-Tick. Abschaltbar über `FAST_APPLY = False` in `config.py`.

## Installation

1. Diesen Ordner `ConduitRemote` kopieren nach:
   - Windows: `\Users\[user]\Documents\Ableton\User Library\Remote Scripts\`
   - macOS: `~/Music/Ableton/User Library/Remote Scripts/`
2. Live neu starten.
3. Preferences → Link/Tempo/MIDI → Control Surface → **ConduitRemote** wählen.

Ports (in `config.py`): lauscht auf **9010**, antwortet an Absender-IP:**9011**.
Bewusst getrennt von Conduits eigenem OSC (9000/9001) und AbletonOSC (11000/11001).

## Protokoll

**Commands (Client → Script)** — AbletonOSC-kompatible Adressen:

| Adresse | Argumente |
|---|---|
| `/live/song/start_playing` · `stop_playing` · `continue_playing` · `undo` · `redo` | — |
| `/live/song/set/tempo` | `f` |
| `/live/song/set/metronome` · `session_record` | `i` |
| `/live/track/set/volume` · `panning` | `track_ref, f` |
| `/live/track/set/send` | `track_ref, i send, f` |
| `/live/track/set/mute` · `solo` · `arm` | `track_ref, i` |
| `/live/return/set/volume` · `panning` | `i return, f` |
| `/live/master/set/volume` · `panning` | `f` |
| `/live/clip_slot/fire` · `stop` | `track_ref, i scene` |
| `/live/scene/fire` | `i scene` |
| `/live/track/stop_all_clips` | `track_ref` |

`track_ref` = Index (int, AbletonOSC-kompatibel) **oder** Stable-ID-String
(`"tr:3"` aus den Snapshots) — Stable-IDs überleben Track-Reorder in Live.

**State (Script → Client)** — Domain-Sync, JSON über OSC:

- `/remote/state/{domain}/get` → Voll-Snapshot anfordern
- `/remote/state/{domain}/subscribe` / `unsubscribe`
- Antworten: `/remote/state/{domain}/snapshot` bzw. `/diff`,
  Argumente `[seq:int, chunk:int, chunks:int, json:str]`
- Diff-Semantik: geänderte Keys → neuer Wert, entfernte Keys → `null`.
  Verlorene Pakete heilt der Client durch Snapshot-Neuanforderung (seq-Lücke).

Domains: `transport` (is_playing, tempo, metronome, session_record, sig) ·
`tracks` (Name, Farbe, Art, Reihenfolge) · `mixer` (vol/pan/sends/mute/solo/arm)
· `session` (Clip-Grid pro Track-Zeile: state stopped/playing/triggered/recording
+ Name/Farbe; Scenes).

**Heartbeat:** Client sendet `/remote/ping` (~2 s) → `/remote/pong [version]`.
Nach ~6 s ohne Ping werden alle Subscriptions beendet.

**Meter (M2)** — Hochraten-Pfad, getrennt von den Domains:

- `/remote/meters/subscribe` / `unsubscribe`
- Push: `/remote/meters` mit flachen Tripeln `[id:str, left:f, right:f] × n`
  (Tracks + Returns + Master, gleiche Stable-IDs wie die Domains; Werte =
  Lives rohe `output_meter`-Norm 0..1) — pro Manager-Tick (~10 Hz), keine
  Sequenznummer (Frames sind idempotent), Stille wird nach einem
  Null-Frame dedupliziert.

## Tests

```
python3 -m pytest tests/ -q     # 120 Tests, läuft ohne Live (Live-API-Stub)
```

## Lizenz-Hinweise

Eigenständige Implementierung. Das Adress-Schema der Commands folgt bewusst
den Konventionen von [AbletonOSC](https://github.com/ideoforms/AbletonOSC)
(MIT, © Daniel Jones). Kein Code aus proprietären Scripts (touchAble, Grip, HK).

## Status / Roadmap

M1a fertig, Meter-Hochraten-Pfad (M2) drin. Es folgen laut `TOUCHLIVE.md`
(im Conduit-Repo: docs/TouchLive.md): Device-Domain generisch (M3),
Browser (M4), bespoke Device-UIs (M5), Modulator-Zwillinge (M6).
