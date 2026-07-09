"""Mixer domain: volume/pan/sends/mute/solo/arm for tracks/returns/master.

Uses the SAME stable ids as sync.tracks.TracksDomain (same underlying Live
objects, same prefixes from sync.stable_ids) so a client can join a track's
structure entry and its mixer entry on one key.

Design choice for the master track: real Live's master track has no mute/
solo/arm and no sends of its own (only volume/pan are meaningful), so its
mixer entry OMITS the "sends"/"mute"/"solo"/"arm" keys entirely rather than
padding them with placeholder values a client might mistake for real state.
Regular tracks and return tracks both get the full key set.
"""

from .base import Domain
from . import stable_ids


class MixerDomain(Domain):
    NAME = "mixer"

    def __init__(self, live_song, sender):
        Domain.__init__(self, live_song, sender)
        self._structure_bound = False
        self._track_bindings = []   # list of _TrackMixerBinding

    # -- snapshot -------------------------------------------------------------

    def collect(self):
        song = self.song
        result = {}
        for track in song.tracks:
            sid = stable_ids.get_id(track, stable_ids.TRACK_PREFIX)
            result[sid] = _full_state(track)
        for track in song.return_tracks:
            sid = stable_ids.get_id(track, stable_ids.RETURN_PREFIX)
            result[sid] = _full_state(track)
        master = song.master_track
        sid = stable_ids.get_id(master, stable_ids.MASTER_PREFIX)
        result[sid] = _master_state(master)
        return result

    # -- listener plumbing ------------------------------------------------------

    def _all_tracks(self):
        song = self.song
        tracks = list(song.tracks) + list(song.return_tracks)
        tracks.append(song.master_track)
        return tracks

    def _on_structure_change(self):
        self._rebind_track_listeners()
        self.mark_dirty()

    def _on_field_change(self):
        self.mark_dirty()

    def _rebind_track_listeners(self):
        for binding in self._track_bindings:
            binding.unbind()
        self._track_bindings = []
        for track in self._all_tracks():
            binding = _TrackMixerBinding(track, self._on_field_change)
            binding.bind()
            self._track_bindings.append(binding)

    def on_attach(self):
        if self._structure_bound:
            return
        song = self.song
        song.add_tracks_listener(self._on_structure_change)
        song.add_return_tracks_listener(self._on_structure_change)
        self._rebind_track_listeners()
        self._structure_bound = True

    def on_detach(self):
        if not self._structure_bound:
            return
        song = self.song
        song.remove_tracks_listener(self._on_structure_change)
        song.remove_return_tracks_listener(self._on_structure_change)
        for binding in self._track_bindings:
            binding.unbind()
        self._track_bindings = []
        self._structure_bound = False


def _full_state(track):
    md = track.mixer_device
    return {
        "vol": float(md.volume.value),
        "pan": float(md.panning.value),
        "sends": [float(p.value) for p in md.sends],
        "mute": bool(track.mute),
        "solo": bool(track.solo),
        "arm": bool(track.arm),
    }


def _master_state(track):
    md = track.mixer_device
    return {
        "vol": float(md.volume.value),
        "pan": float(md.panning.value),
    }


class _TrackMixerBinding(object):
    """Binds volume/panning/sends/mute/solo/arm value listeners on one
    track's mixer device.

    We bind mute/solo/arm/send listeners on every track uniformly (master
    included) purely as change hints that call mark_dirty() -- collect()
    remains the single source of truth for what actually gets serialised,
    and _master_state() above simply never reads those fields for the
    master track. Binding a listener Live will never fire for master is
    harmless; it costs nothing at runtime.
    """

    def __init__(self, track, on_change):
        self.track = track
        self.on_change = on_change
        self._send_cbs = []

    def bind(self):
        track = self.track
        md = track.mixer_device
        md.volume.add_value_listener(self.on_change)
        md.panning.add_value_listener(self.on_change)
        for send in md.sends:
            send.add_value_listener(self.on_change)
            self._send_cbs.append(send)
        track.add_mute_listener(self.on_change)
        track.add_solo_listener(self.on_change)
        track.add_arm_listener(self.on_change)

    def unbind(self):
        track = self.track
        md = track.mixer_device
        md.volume.remove_value_listener(self.on_change)
        md.panning.remove_value_listener(self.on_change)
        for send in self._send_cbs:
            send.remove_value_listener(self.on_change)
        self._send_cbs = []
        track.remove_mute_listener(self.on_change)
        track.remove_solo_listener(self.on_change)
        track.remove_arm_listener(self.on_change)
