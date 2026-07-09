"""Transport domain: playback, tempo, metronome, session record, time sig."""

from .base import Domain


class TransportDomain(Domain):
    NAME = "transport"

    def __init__(self, live_song, sender):
        Domain.__init__(self, live_song, sender)
        self._bound = False

    # -- snapshot -------------------------------------------------------------

    def collect(self):
        song = self.song
        return {
            "is_playing": bool(song.is_playing),
            "tempo": float(song.tempo),
            "metronome": bool(song.metronome),
            "session_record": bool(song.session_record),
            "sig_num": int(song.signature_numerator),
            "sig_den": int(song.signature_denominator),
        }

    # -- listeners --------------------------------------------------------------

    def _on_change(self):
        self.mark_dirty()

    def on_attach(self):
        if self._bound:
            return
        song = self.song
        song.add_is_playing_listener(self._on_change)
        song.add_tempo_listener(self._on_change)
        song.add_metronome_listener(self._on_change)
        song.add_session_record_listener(self._on_change)
        self._bound = True

    def on_detach(self):
        if not self._bound:
            return
        song = self.song
        song.remove_is_playing_listener(self._on_change)
        song.remove_tempo_listener(self._on_change)
        song.remove_metronome_listener(self._on_change)
        song.remove_session_record_listener(self._on_change)
        self._bound = False
