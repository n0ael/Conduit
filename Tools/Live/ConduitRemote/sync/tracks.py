"""Tracks domain: track/return/master STRUCTURE only (name/color/kind/index).

Mixer values (volume/pan/sends/mute/solo/arm) live in sync.mixer.MixerDomain
and share the exact same stable ids (see sync.stable_ids) so a client can
join structure and mixer state on the same key.
"""

from .base import Domain
from . import stable_ids


class TracksDomain(Domain):
    NAME = "tracks"

    def __init__(self, live_song, sender):
        Domain.__init__(self, live_song, sender)
        self._structure_bound = False
        self._track_bindings = []   # list of (track, name_cb, color_cb)

    # -- snapshot -------------------------------------------------------------

    def collect(self):
        song = self.song
        result = {}
        for index, track in enumerate(song.tracks):
            sid = stable_ids.get_id(track, stable_ids.TRACK_PREFIX)
            result[sid] = {
                "name": track.name,
                "color": track.color,
                "kind": "midi" if track.has_midi_input else "audio",
                "index": index,
            }
        for index, track in enumerate(song.return_tracks):
            sid = stable_ids.get_id(track, stable_ids.RETURN_PREFIX)
            result[sid] = {
                "name": track.name,
                "color": track.color,
                "kind": "return",
                "index": index,
            }
        master = song.master_track
        sid = stable_ids.get_id(master, stable_ids.MASTER_PREFIX)
        result[sid] = {
            "name": master.name,
            "color": master.color,
            "kind": "master",
            "index": 0,
        }
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
        for track, name_cb, color_cb in self._track_bindings:
            track.remove_name_listener(name_cb)
            track.remove_color_listener(color_cb)
        self._track_bindings = []
        for track in self._all_tracks():
            name_cb = self._on_field_change
            color_cb = self._on_field_change
            track.add_name_listener(name_cb)
            track.add_color_listener(color_cb)
            self._track_bindings.append((track, name_cb, color_cb))

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
        for track, name_cb, color_cb in self._track_bindings:
            track.remove_name_listener(name_cb)
            track.remove_color_listener(color_cb)
        self._track_bindings = []
        self._structure_bound = False
