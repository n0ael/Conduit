"""ConduitRemote control surface: wires OSC server, commands and sync domains
into Live's ControlSurface lifecycle (schedule_message-driven tick loop).
"""

import logging

from . import config
from .osc.server import OscServer
from .handlers import FAST_WHITELIST
from .handlers.registry import CommandContext, CommandRegistry
from .handlers import song as song_handlers
from .handlers import track as track_handlers
from .handlers import session as session_handlers
from .heartbeat import Heartbeat
from .sync.base import to_json
from .sync.delivery import Sender
from .sync import stable_ids

logger = logging.getLogger(__name__)

_MAX_DOMAIN_FAILURES = 5

try:
    from ableton.v2.control_surface import ControlSurface
except ImportError:
    class ControlSurface(object):
        """Minimal stand-in so Manager is importable/instantiable outside
        Live's Python environment (unit tests)."""

        def __init__(self, c_instance):
            self._c_instance = c_instance
            self.scheduled = []

        def schedule_message(self, delay, fn):
            self.scheduled.append((delay, fn))

        def show_message(self, message):
            return None

        def disconnect(self):
            return None


class ServerSenderAdapter(object):
    """Adapts OscServer.send to the `sender.send_json(address, seq, payload)`
    protocol sync.base.Domain expects. args layout is
    [seq, chunk_index, chunk_count, json] - chunk fields are fixed at 1/1
    for now (no payload splitting yet); delivery.py may replace this
    adapter entirely at integration time."""

    def __init__(self, server):
        self._server = server

    def send_json(self, address, seq, payload, force=False):
        json_str = to_json(payload)
        self._server.send(address, [seq, 1, 1, json_str])


def _default_domain_factory(song, sender):
    from .sync import build_domains
    return build_domains(song, sender)


class Manager(ControlSurface):
    def __init__(self, c_instance, song=None, domain_factory=None):
        ControlSurface.__init__(self, c_instance)
        self._c_instance = c_instance
        self._song = song if song is not None else c_instance.song()
        self._tick_count = 0
        self._domain_failures = {}

        self.server = OscServer(
            config.BIND_HOST, config.LISTEN_PORT, config.RESPONSE_PORT,
            fast_apply=config.FAST_APPLY)

        # Real delivery layer: JSON chunking + per-address dedupe.
        # (ServerSenderAdapter above is kept as a minimal fallback/reference.)
        self._sender = Sender(self.server.send)

        factory = domain_factory if domain_factory is not None else _default_domain_factory
        self.domains = factory(self._song, self._sender)

        self.heartbeat = Heartbeat(on_timeout=self._on_heartbeat_timeout)

        self.registry = CommandRegistry()
        self.ctx = CommandContext(self._get_song,
                                  track_resolver=self._resolve_stable_id)
        song_handlers.register_all(self.registry)
        track_handlers.register_all(self.registry)
        session_handlers.register_all(self.registry)

        # Same function for both registries: registry.dispatch() looks the
        # address up itself and is thread-tolerant (read-only handler dict).
        for address in self.registry.addresses():
            self.server.add_handler(address, self._dispatch_command)
        for address in FAST_WHITELIST:
            self.server.set_fast_handler(address, self._dispatch_command)

        for name, domain in list(self.domains.items()):
            self.server.add_handler(
                "/remote/state/%s/get" % name, self._make_snapshot_handler(domain))
            self.server.add_handler(
                "/remote/state/%s/subscribe" % name, self._make_subscribe_handler(domain))
            self.server.add_handler(
                "/remote/state/%s/unsubscribe" % name, self._make_unsubscribe_handler(domain))

        self.server.add_handler("/remote/ping", self._on_ping)

        self.schedule_message(1, self.tick)

    # -- wiring helpers -----------------------------------------------------

    def _get_song(self):
        return self._song

    def _resolve_stable_id(self, ref):
        """Resolve a stable-id string ("tr:3") to a track object.

        Same id space as the sync domains (stable_ids registry), so a client
        holding ids from a tracks snapshot can address tracks robustly even
        after reordering.  Returns None when unknown.
        """
        for track in self._song.tracks:
            if stable_ids.get_id(track, stable_ids.TRACK_PREFIX) == ref:
                return track
        # numeric-string fallback (AbletonOSC-style index as string)
        try:
            index = int(ref)
        except (TypeError, ValueError):
            return None
        tracks = self._song.tracks
        if 0 <= index < len(tracks):
            return tracks[index]
        return None

    def _dispatch_command(self, address, args):
        self.registry.dispatch(address, args, self.ctx)

    @staticmethod
    def _make_snapshot_handler(domain):
        def _handler(address, args):
            domain.send_snapshot()
        return _handler

    @staticmethod
    def _make_subscribe_handler(domain):
        def _handler(address, args):
            domain.on_subscribe()
        return _handler

    @staticmethod
    def _make_unsubscribe_handler(domain):
        def _handler(address, args):
            domain.on_unsubscribe()
        return _handler

    def _on_ping(self, address, args):
        self.heartbeat.note_ping(self._tick_count)
        self.server.send("/remote/pong", [config.PROTOCOL_VERSION])

    def _on_heartbeat_timeout(self):
        logger.info("heartbeat timeout: unsubscribing all domains")
        for domain in list(self.domains.values()):
            try:
                domain.on_unsubscribe()
            except Exception:
                logger.exception(
                    "on_unsubscribe failed for domain %s",
                    getattr(domain, "NAME", "?"))

    # -- lifecycle ------------------------------------------------------------

    def tick(self):
        self._tick_count += 1
        try:
            self.server.process()
        except Exception:
            logger.exception("server.process() failed")

        for name in list(self.domains.keys()):
            domain = self.domains.get(name)
            if domain is None:
                continue
            try:
                domain.on_tick(self._tick_count)
                self._domain_failures[name] = 0
            except Exception:
                count = self._domain_failures.get(name, 0) + 1
                self._domain_failures[name] = count
                logger.exception(
                    "domain %s on_tick failed (%d/%d)",
                    name, count, _MAX_DOMAIN_FAILURES)
                if count > _MAX_DOMAIN_FAILURES:
                    logger.error(
                        "domain %s exceeded failure budget, dropping", name)
                    try:
                        domain.detach()
                    except Exception:
                        logger.exception("domain %s detach failed", name)
                    del self.domains[name]
                    del self._domain_failures[name]

        self.heartbeat.check(self._tick_count)
        self.schedule_message(config.TICK_INTERVAL, self.tick)

    def disconnect(self):
        for domain in list(self.domains.values()):
            try:
                domain.on_unsubscribe()
            except Exception:
                logger.exception("on_unsubscribe failed during disconnect")
            try:
                domain.detach()
            except Exception:
                logger.exception("detach failed during disconnect")
        self.domains = {}
        self.server.shutdown()
        ControlSurface.disconnect(self)
