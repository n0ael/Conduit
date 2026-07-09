"""Client liveness tracking via periodic /remote/ping.

note_ping(tick) is called whenever a ping arrives; check(tick) is called
every manager tick and flips `alive` False - firing on_timeout exactly
once per True->False transition - once too many ticks have passed without
a ping.
"""

from . import config


class Heartbeat(object):
    def __init__(self, timeout_ticks=None, on_timeout=None):
        self._timeout_ticks = (
            config.HEARTBEAT_TIMEOUT_TICKS if timeout_ticks is None
            else timeout_ticks)
        self.on_timeout = on_timeout
        self._last_ping_tick = None
        self.alive = False

    def note_ping(self, tick):
        self._last_ping_tick = tick
        self.alive = True

    def check(self, tick):
        if self._last_ping_tick is None:
            alive_now = False
        else:
            alive_now = (tick - self._last_ping_tick) <= self._timeout_ticks

        was_alive = self.alive
        self.alive = alive_now
        if was_alive and not alive_now and self.on_timeout is not None:
            try:
                self.on_timeout()
            except Exception:
                pass
        return self.alive
