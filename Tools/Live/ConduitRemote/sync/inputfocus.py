"""Conduit Grid track focus + follow-selection routing (Conduit Block H v2).

Live-performance goal: Conduit's MPE grid and the master MIDI device (Push,
keyboard, ...) play DIFFERENT tracks at the same time, freely switchable.

Focus semantics (set_focus):
  * focus track: input routing = grid_input (Conduit's grid MIDI-out port),
    monitor IN - it always hears Conduit, regardless of arm/selection.
  * every OTHER midi track whose input is "All Ins": monitor OFF ("All Ins"
    includes the Conduit port - Auto would leak grid notes when armed).
    Tracks with an explicit input routing are left alone.
  * Live's currently SELECTED track (midi, on "All Ins", not the focus):
    input = master_input (e.g. "FromPush"), monitor AUTO - remembered as
    the "moved" track so it can be restored later.
  * a PREVIOUS focus track is restored (input back to "All Ins", monitor
    OFF) but only if its input still is the grid port - i.e. only if WE
    set it; user re-routings are never overwritten.

Follow selection (selected_track listener + manager-tick poll, active only
while follow is on AND a focus exists) is STATE-BASED (Feldtest-Runde 3,
11.07.2026): every routing pass decides per track from its CURRENT input -
"All Ins" and master_input/grid_input are "ours" (managed: deselected ->
"All Ins" + OFF, selected -> master_input + AUTO), any OTHER input
(sequencer, hardware port, ...) is never touched, neither input nor
monitor (User-Regel: ein sequencer-gespeister Track darf durch Selektion
nichts verlieren). No move-tracking -> self-healing and idempotent.

"All Ins" detection is robust against localisation: entry 0 of
available_input_routing_types IS "All Ins" in Live - matched via
display_name of that first entry, never via a hardcoded string.

All LOM access is guarded (docs/TouchLive.md par.10d fallen); the state
lives only in this script session - a Live restart clears the focus, which
the client observes through the tracks domain going empty on conduit_focus.
"""

import logging

from . import stable_ids

logger = logging.getLogger(__name__)

# Live.Track.Track.monitoring_states: 0 = In, 1 = Auto, 2 = Off
MONITOR_IN = 0
MONITOR_AUTO = 1
MONITOR_OFF = 2


def find_routing_type(track, wanted_name):
    """available_input_routing_types by display_name: exact match first,
    then case-insensitive prefix (Live appends suffixes like " (Port 1)").
    None when nothing matches / routing unavailable."""
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


def apply_track_input(track, monitor_state, input_name):
    """Set a track's monitor + input routing; every sub-operation is
    LOM-defensive, an empty input_name leaves the routing untouched.
    Writes only on actual change (idempotenter Routing-Pass darf Lives
    Undo-/Listener-Maschinerie nicht mit No-op-Writes fluten)."""
    try:
        if track.current_monitoring_state != monitor_state:
            track.current_monitoring_state = monitor_state
    except Exception:
        logger.debug("input focus: monitoring not settable on %r",
                     getattr(track, "name", "?"))
    routing = find_routing_type(track, input_name)
    if routing is None:
        return
    try:
        wanted = getattr(routing, "display_name", None)
        if _current_input_name(track) != wanted:
            track.input_routing_type = routing
    except Exception:
        logger.debug("input focus: input routing not settable on %r",
                     getattr(track, "name", "?"))


def _all_ins_name(track):
    """display_name of routing entry 0 ("All Ins"), or None."""
    try:
        available = list(track.available_input_routing_types)
    except Exception:
        return None
    if not available:
        return None
    return getattr(available[0], "display_name", None)


def _current_input_name(track):
    try:
        return getattr(track.input_routing_type, "display_name", None)
    except Exception:
        return None


def _is_on_all_ins(track):
    name = _all_ins_name(track)
    return name is not None and _current_input_name(track) == name


def _is_midi(track):
    try:
        return bool(track.has_midi_input)
    except Exception:
        return False


class InputFocusService(object):
    def __init__(self, song):
        self._song = song
        self._focus_key = None    # stable_ids._identity of the focus track
        self._follow = True
        self._grid_input = ""
        self._master_input = ""
        self._listener_bound = False
        self._last_selected_key = None   # poll()-Dedupe (Feldtest 11.07.2026)

        # wired by the manager: tracks domain mark_dirty
        self.on_state_changed = None

    # -- queries (tracks domain) ----------------------------------------------

    def focus_stable_id(self):
        """Stable-ID of the focus track, "" when none / track vanished."""
        track = self._find_by_key(self._focus_key)
        if track is None:
            return ""
        return stable_ids.get_id(track, stable_ids.TRACK_PREFIX)

    def follow_enabled(self):
        return self._follow

    # -- commands ---------------------------------------------------------------

    def set_follow(self, enabled):
        self._follow = bool(enabled)
        self._notify()

    def set_focus(self, target, grid_input, master_input, follow):
        """Route target to the grid (monitor IN + grid_input), then apply
        the state-based routing rules to everything else."""
        if target is None:
            return
        self._grid_input = str(grid_input or "")
        self._master_input = str(master_input or "")
        self._follow = bool(follow)

        self._focus_key = stable_ids._identity(target)
        self._last_selected_key = self._selected_key()
        self._ensure_listener()

        # Diagnose (Feldtest-Runde 4): einmal pro Fokus ins Live-Log --
        # zeigt follow/grid/master und ob der Listener gebunden ist.
        logger.info(
            "input focus rev4: focus=%s grid=%r master=%r follow=%s listener=%s",
            self.focus_stable_id(), self._grid_input, self._master_input,
            self._follow, self._listener_bound)

        self._apply_routing()
        self._notify()

    def _apply_routing(self):
        """State-based routing pass (Feldtest-Runde 3, 11.07.2026 --
        ersetzt das moved-Tracking, dadurch selbstheilend/idempotent).
        Pro MIDI-Track entscheidet der IST-Input:

          * focus track          -> grid_input + monitor IN
          * selected (not focus) -> if on "All Ins" OR master_input:
                                    master_input + AUTO (playable)
          * everything else      -> if on grid_input or master_input
                                    (i.e. WE routed it earlier): back to
                                    "All Ins" + OFF; if on "All Ins":
                                    monitor OFF (input untouched)
          * any OTHER input (sequencer, hardware, ...) -> NEVER touched,
            neither input nor monitor (User-Regel 11.07.2026: ein Track,
            den ein Sequencer speist, darf durch Selektion nichts
            verlieren).
        """
        if self._focus_key is None:
            return

        selected_key = self._selected_key()

        try:
            tracks = list(self._song.tracks)
        except Exception:
            return

        for track in tracks:
            if not _is_midi(track):
                continue
            key = stable_ids._identity(track)
            if key == self._focus_key:
                apply_track_input(track, MONITOR_IN, self._grid_input)
            elif key == selected_key:
                if (_is_on_all_ins(track)
                        or self._input_matches(track, self._master_input)):
                    apply_track_input(track, MONITOR_AUTO, self._master_input)
            elif (self._input_matches(track, self._grid_input)
                  or self._input_matches(track, self._master_input)):
                # von UNS geroutet (alter Fokus / alte Selektion) -> aufraeumen
                all_ins = _all_ins_name(track)
                apply_track_input(track, MONITOR_OFF, all_ins or "")
            elif _is_on_all_ins(track):
                apply_track_input(track, MONITOR_OFF, "")

    # -- follow selection ---------------------------------------------------------

    def poll(self):
        """[Manager-Tick, ~100 ms] Selektions-Wechsel erkennen — der
        ROBUSTE Follow-Pfad (Feldtest 11.07.2026: der selected_track-
        Listener kann in Live still ausfallen, dann blieb der zuvor
        selektierte Track auf dem Master-Input hängen). Der Listener
        bleibt als Schnellpfad; das _last_selected_key-Dedupe verhindert
        Doppel-Feuern."""
        self._handle_selection_change()

    def on_selection_changed(self):
        """selected_track listener body (Schnellpfad; auch Tests)."""
        self._handle_selection_change()

    def _handle_selection_change(self):
        selected_key = self._selected_key()

        if selected_key == self._last_selected_key:
            return   # kein Wechsel (oder Listener war schneller als poll)
        self._last_selected_key = selected_key

        # Diagnose (Feldtest-Runde 4): jeder erkannte Selektionswechsel.
        logger.info("input focus rev4: selection change -> %s (follow=%s focus=%s)",
                    selected_key, self._follow, self._focus_key is not None)

        # Zustandsbasierter Routing-Pass: raeumt den vorher selektierten
        # Track auf (master -> All Ins + Off, All Ins -> Off) und bewegt
        # die neue Selektion -- fremde Inputs bleiben komplett unberuehrt.
        if self._follow and self._focus_key is not None:
            self._apply_routing()
        self._notify()   # selected-Feld der Domain in jedem Fall frisch

    # -- lifecycle ---------------------------------------------------------------

    def detach(self):
        if not self._listener_bound:
            return
        try:
            self._song.view.remove_selected_track_listener(
                self.on_selection_changed)
        except Exception:
            logger.exception("input focus: listener unbind failed")
        self._listener_bound = False

    # -- internals ---------------------------------------------------------------

    def _ensure_listener(self):
        if self._listener_bound:
            return
        try:
            self._song.view.add_selected_track_listener(
                self.on_selection_changed)
            self._listener_bound = True
        except Exception:
            # Kein Listener = kein Follow (Fokus-Command funktioniert
            # trotzdem); Muster Domain.attach-Poll-Fallback, hier ohne Poll.
            logger.exception("input focus: selection listener unavailable")

    def _selected_track(self):
        try:
            return self._song.view.selected_track
        except Exception:
            return None

    def _find_by_key(self, key):
        if key is None:
            return None
        try:
            tracks = list(self._song.tracks)
        except Exception:
            return None
        for track in tracks:
            if stable_ids._identity(track) == key:
                return track
        return None

    def _selected_key(self):
        selected = self._selected_track()
        return stable_ids._identity(selected) if selected is not None else None

    def _input_matches(self, track, wanted_name):
        """IST-Input des Tracks == wanted_name (exakt oder Praefix, wie das
        Routing-Matching -- Live haengt Port-Suffixe an)? False bei leerem
        wanted_name oder unlesbarem Routing."""
        if not wanted_name:
            return False
        current = _current_input_name(track)
        if current is None:
            return False
        return (current == wanted_name
                or current.lower().startswith(wanted_name.lower()))

    def _notify(self):
        if self.on_state_changed is not None:
            try:
                self.on_state_changed()
            except Exception:
                logger.exception("input focus: state notify failed")
