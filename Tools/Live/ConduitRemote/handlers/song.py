"""/live/song/* command handlers: transport + global song settings."""

import logging

from . import _util
from ..sync import stable_ids

logger = logging.getLogger(__name__)

_TEMPO_MIN = 20.0
_TEMPO_MAX = 999.0


def _start_playing(ctx, args):
    song = ctx.song
    if song is None:
        return
    song.start_playing()


def _stop_playing(ctx, args):
    song = ctx.song
    if song is None:
        return
    song.stop_playing()


def _continue_playing(ctx, args):
    song = ctx.song
    if song is None:
        return
    fn = getattr(song, "continue_playing", None)
    if callable(fn):
        fn()
    else:
        logger.debug("continue_playing: not supported by this song object")


def _set_tempo(ctx, args):
    if len(args) < 1:
        return
    song = ctx.song
    if song is None:
        return
    song.tempo = _util.clamp(
        _util.as_float(args[0], song.tempo), _TEMPO_MIN, _TEMPO_MAX)


def _set_metronome(ctx, args):
    if len(args) < 1:
        return
    song = ctx.song
    if song is None:
        return
    song.metronome = _util.as_bool(args[0])


def _set_session_record(ctx, args):
    if len(args) < 1:
        return
    song = ctx.song
    if song is None:
        return
    song.session_record = _util.as_bool(args[0])


def _undo(ctx, args):
    song = ctx.song
    if song is None:
        return
    fn = getattr(song, "undo", None)
    if callable(fn):
        fn()


def _redo(ctx, args):
    song = ctx.song
    if song is None:
        return
    fn = getattr(song, "redo", None)
    if callable(fn):
        fn()


def _set_selected_track(ctx, args):
    """View-Selektion (M4: Browser-Load-Ziel) — track_ref wie ueberall
    (Index oder Stable-ID); LOM-sicher via try/except."""
    if len(args) < 1:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("selected_track: unknown track %r", args[0])
        return
    try:
        ctx.song.view.selected_track = track
    except Exception:
        logger.exception("selected_track failed")


# Live.Track.Track.monitoring_states: 0 = In, 1 = Auto, 2 = Off
_MONITOR_IN = 0
_MONITOR_AUTO = 1


def _find_routing_type(track, wanted_name):
    """available_input_routing_types nach display_name durchsuchen:
    exakter Treffer zuerst, dann case-insensitive Praefix (Live haengt
    Suffixe wie ' (Port 1)' an). None wenn nichts passt."""
    if not wanted_name:
        return None
    try:
        available = list(track.available_input_routing_types)
    except Exception:
        return None
    for routing in available:
        if getattr(routing, "display_name", None) == wanted_name:
            return routing
    lowered = wanted_name.lower()
    for routing in available:
        display = getattr(routing, "display_name", "") or ""
        if display.lower().startswith(lowered):
            return routing
    return None


def _apply_track_input(track, monitor_state, input_name):
    """Monitor + Input-Routing eines Tracks setzen — jede Teiloperation
    LOM-defensiv (Fallen-Regel docs/TouchLive.md 10d), leerer input_name
    laesst das Routing unangetastet."""
    try:
        track.current_monitoring_state = monitor_state
    except Exception:
        logger.debug("midi_input_focus: monitoring not settable on %r",
                     getattr(track, "name", "?"))
    routing = _find_routing_type(track, input_name)
    if routing is None:
        return
    try:
        track.input_routing_type = routing
    except Exception:
        logger.debug("midi_input_focus: input routing not settable on %r",
                     getattr(track, "name", "?"))


def _set_midi_input_focus(ctx, args):
    """Block H (Conduit Grid-Track-Selector): args = [track_ref,
    input_name, default_input_name]. Der Ziel-Track bekommt Monitor "In"
    plus input_name als MIDI-Input (Conduits Grid-MIDI-Out, z. B.
    "Conduit A"); alle ANDEREN MIDI-Tracks Monitor "Auto" plus
    default_input_name (leer = Routing unangetastet). Audio-Tracks/
    Returns/Master bleiben unberuehrt."""
    if len(args) < 2:
        return
    song = ctx.song
    if song is None:
        return
    target = ctx.resolve_track(args[0])
    if target is None:
        logger.debug("midi_input_focus: unknown track %r", args[0])
        return
    input_name = str(args[1] or "")
    default_name = str(args[2] or "") if len(args) > 2 else ""

    # LOM-Wrapper sind NICHT identitaetsstabil (Feldtest 09.07.2026) --
    # Ziel-Vergleich ueber die _live_ptr-Identitaet, nie ueber ==/is.
    target_key = stable_ids._identity(target)

    for track in song.tracks:
        try:
            is_midi = bool(track.has_midi_input)
        except Exception:
            is_midi = False
        if not is_midi:
            continue
        if stable_ids._identity(track) == target_key:
            _apply_track_input(track, _MONITOR_IN, input_name)
        else:
            _apply_track_input(track, _MONITOR_AUTO, default_name)


def register_all(registry):
    registry.register("/live/song/start_playing", _start_playing)
    registry.register("/live/song/stop_playing", _stop_playing)
    registry.register("/live/song/continue_playing", _continue_playing)
    registry.register("/live/song/set/tempo", _set_tempo)
    registry.register("/live/song/set/metronome", _set_metronome)
    registry.register("/live/song/set/session_record", _set_session_record)
    registry.register("/live/song/undo", _undo)
    registry.register("/live/song/redo", _redo)
    registry.register("/live/song/set/selected_track", _set_selected_track)
    registry.register("/live/song/set/midi_input_focus", _set_midi_input_focus)
