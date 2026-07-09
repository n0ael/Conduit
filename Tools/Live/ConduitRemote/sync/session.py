"""Session domain: scenes + the session clip grid.

DESIGN NOTE ON SHAPE -- read before touching collect():

sync.base.compute_diff() is a *shallow* dict diff: it walks top-level keys
and, if a value differs (by !=), resends that ENTIRE value. The "obvious"
shape for session state is nested:

    {"scenes": {sid: {...}, ...}, "grid": {tid: [row], ...}}

But that shape has exactly two top-level keys, "scenes" and "grid" -- so a
single clip changing playback state would make compute_diff() decide the
*whole* "grid" value differs and re-serialise/re-send every track's entire
row, every tick, forever. To keep diffs small we instead flatten collect()
into ONE dict with composite string keys:

    "scene:{scene_stable_id}" -> {"name": ..., "color": ..., "index": ...}
    "grid:{track_stable_id}"  -> [slot_state, slot_state, ...]   (scene order)

Now the shallow top-level diff IS the granularity we want: touching one
scene only changes its "scene:{sid}" key; touching one clip only changes
its track's "grid:{tid}" row (all other tracks' rows are untouched keys and
therefore never re-sent). Removing a scene/track makes that composite key
vanish from the new collect() output, which compute_diff() correctly turns
into {"scene:{sid}": None} / {"grid:{tid}": None} for the client to delete.
This is a real (if coarse -- per-row, not per-cell) improvement over the
nested shape, achieved with zero changes to compute_diff() itself.
"""

from .base import Domain
from . import stable_ids

_CLIP_PROPS = ("name", "color", "is_playing", "is_triggered", "is_recording")


class SessionDomain(Domain):
    NAME = "session"

    def __init__(self, live_song, sender):
        Domain.__init__(self, live_song, sender)
        self._structure_bound = False
        self._scene_bindings = []   # list of (scene, name_cb, color_cb)
        self._slot_bindings = []    # list of _SlotBinding

    # -- snapshot -------------------------------------------------------------

    def collect(self):
        song = self.song
        result = {}
        for index, scene in enumerate(song.scenes):
            sid = stable_ids.get_id(scene, stable_ids.SCENE_PREFIX)
            result["scene:%s" % sid] = {
                "name": scene.name,
                "color": scene.color,
                "index": index,
            }
        for track in song.tracks:
            tid = stable_ids.get_id(track, stable_ids.TRACK_PREFIX)
            result["grid:%s" % tid] = [
                _slot_state(slot) for slot in track.clip_slots
            ]
        return result

    # -- listener plumbing ------------------------------------------------------

    def _on_structure_change(self):
        self._rebind()
        self.mark_dirty()

    def _on_field_change(self):
        self.mark_dirty()

    def _rebind(self):
        self._unbind_scenes()
        self._unbind_slots()
        self._bind_scenes()
        self._bind_slots()

    def _unbind_scenes(self):
        for scene, name_cb, color_cb in self._scene_bindings:
            try:
                scene.remove_name_listener(name_cb)
                scene.remove_color_listener(color_cb)
            except Exception:
                pass   # Scene existiert nicht mehr (Delete/Teardown)
        self._scene_bindings = []

    def _bind_scenes(self):
        for scene in self.song.scenes:
            name_cb = self._on_field_change
            color_cb = self._on_field_change
            scene.add_name_listener(name_cb)
            scene.add_color_listener(color_cb)
            self._scene_bindings.append((scene, name_cb, color_cb))

    def _unbind_slots(self):
        for binding in self._slot_bindings:
            binding.unbind()
        self._slot_bindings = []

    def _bind_slots(self):
        for track in self.song.tracks:
            for slot in track.clip_slots:
                binding = _SlotBinding(slot, self._on_field_change)
                binding.bind()
                self._slot_bindings.append(binding)

    def on_attach(self):
        if self._structure_bound:
            return
        song = self.song
        song.add_scenes_listener(self._on_structure_change)
        song.add_tracks_listener(self._on_structure_change)
        self._bind_scenes()
        self._bind_slots()
        self._structure_bound = True

    def on_detach(self):
        if not self._structure_bound:
            return
        song = self.song
        song.remove_scenes_listener(self._on_structure_change)
        song.remove_tracks_listener(self._on_structure_change)
        self._unbind_scenes()
        self._unbind_slots()
        self._structure_bound = False


def _slot_state(slot):
    if not slot.has_clip:
        return None
    clip = slot.clip
    if clip.is_recording:
        state = "recording"
    elif clip.is_triggered:
        state = "triggered"
    elif clip.is_playing:
        state = "playing"
    else:
        state = "stopped"
    return {"name": clip.name, "color": clip.color, "state": state}


class _SlotBinding(object):
    """Binds has_clip (+ playing_status, if the object supports it) on a
    clip slot, and name/color/is_playing/is_triggered/is_recording on
    whatever clip currently occupies it -- rebinding the clip listeners
    whenever has_clip fires (i.e. a clip was created/deleted in the slot)."""

    def __init__(self, slot, on_change):
        self.slot = slot
        self.on_change = on_change
        self._clip = None
        self._clip_cbs = []
        self._has_clip_cb = None
        self._playing_status_bound = False

    def bind(self):
        self._has_clip_cb = self._on_has_clip_change
        self.slot.add_has_clip_listener(self._has_clip_cb)
        try:
            self.slot.add_playing_status_listener(self.on_change)
            self._playing_status_bound = True
        except Exception:
            # some Live versions/slot types don't expose this listener (or
            # reject the signature, Live 12.4b: Boost ArgumentError);
            # name/color/is_playing/is_triggered/is_recording listeners on
            # the clip itself are sufficient without it.
            self._playing_status_bound = False
        self._bind_clip()

    def unbind(self):
        if self._has_clip_cb is not None:
            try:
                self.slot.remove_has_clip_listener(self._has_clip_cb)
            except Exception:
                pass   # Slot existiert nicht mehr (Delete/Teardown)
            self._has_clip_cb = None
        if self._playing_status_bound:
            try:
                self.slot.remove_playing_status_listener(self.on_change)
            except Exception:
                pass
            self._playing_status_bound = False
        self._unbind_clip()

    def _on_has_clip_change(self):
        self._unbind_clip()
        self._bind_clip()
        self.on_change()

    def _bind_clip(self):
        clip = self.slot.clip
        if clip is None:
            self._clip = None
            self._clip_cbs = []
            return
        cbs = []
        for prop in _CLIP_PROPS:
            cb = self.on_change
            try:
                getattr(clip, "add_%s_listener" % prop)(cb)
                cbs.append((prop, cb))
            except Exception:
                # Live 12.4b wirft u. a. "Observer already connected"
                # (Log-Befund 10.07.2026) — Listener ist dann ohnehin dran
                pass
        self._clip = clip
        self._clip_cbs = cbs

    def _unbind_clip(self):
        if self._clip is not None:
            for prop, cb in self._clip_cbs:
                try:
                    getattr(self._clip, "remove_%s_listener" % prop)(cb)
                except Exception:
                    pass   # Clip existiert nicht mehr
        self._clip = None
        self._clip_cbs = []
