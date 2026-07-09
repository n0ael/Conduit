"""UDP OSC server with an optional fast-apply receiver thread.

Two dispatch paths, mirroring config.FAST_APPLY (see config.py docstring
for the full rationale):

  - fast handlers: a small whitelist of pure-value-write addresses that are
    safe to apply directly from the receiver thread so faders feel
    immediate instead of stepping at Live's ~100 ms scheduler tick.
  - normal handlers: everything else is queued and drained from process(),
    which is called from the main-thread tick.

When fast_apply is False no thread is started at all; process() itself
does the non-blocking socket read and every message (including addresses
that would otherwise be fast-whitelisted) is queued and dispatched through
the normal handler registry - the "everything is queued" classic-script
fallback.
"""

import collections
import errno
import logging
import select
import socket
import threading

from .. import config
from . import codec

logger = logging.getLogger(__name__)

_ECONNRESET_ERRNOS = set([10054])  # WSAECONNRESET
try:
    _ECONNRESET_ERRNOS.add(errno.ECONNRESET)
except AttributeError:
    pass


def _is_connreset(exc):
    """True if exc represents a UDP 'ICMP port unreachable' bounce.

    On Windows an unconnected UDP socket can raise WSAECONNRESET (10054)
    out of recvfrom() after a previous sendto() hit a closed remote port.
    It is not a real error for a connectionless socket - ignore & continue.
    """
    if getattr(exc, "errno", None) in _ECONNRESET_ERRNOS:
        return True
    if getattr(exc, "winerror", None) == 10054:
        return True
    return False


def _chunk_messages(encoded_messages, max_bytes):
    """Greedily pack pre-encoded OSC messages into bundle-sized chunks."""
    chunks = []
    current = []
    current_size = 16  # "#bundle\0" + 8-byte timetag
    for msg in encoded_messages:
        cost = 4 + len(msg)  # int32 size prefix + message bytes
        if current and current_size + cost > max_bytes:
            chunks.append(current)
            current = []
            current_size = 16
        current.append(msg)
        current_size += cost
    if current:
        chunks.append(current)
    return chunks


class OscServer(object):
    def __init__(self, bind_host, listen_port, response_port, fast_apply=True):
        self._response_port = response_port
        self._fast_apply = fast_apply

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((bind_host, listen_port))
        self._sock.setblocking(False)

        self._normal_handlers = {}
        self._fast_handlers = {}
        self._queue = collections.deque()
        self._unknown_logged = set()

        self._client_ip = None
        self._client_lock = threading.Lock()

        self._closed = False
        self._stop_event = threading.Event()
        self._thread = None
        if self._fast_apply:
            self._thread = threading.Thread(
                target=self._recv_loop, name="ConduitRemote-OSC-RX")
            self._thread.daemon = True
            self._thread.start()

    # -- introspection ------------------------------------------------------

    def getsockname(self):
        return self._sock.getsockname()

    @property
    def client_addr(self):
        """(ip, response_port) of the most recent inbound sender, or None."""
        with self._client_lock:
            ip = self._client_ip
        if ip is None:
            return None
        return (ip, self._response_port)

    def _note_client(self, addr):
        with self._client_lock:
            self._client_ip = addr[0]

    # -- registration ---------------------------------------------------------

    def add_handler(self, address, fn):
        self._normal_handlers[address] = fn

    def set_fast_handler(self, address, fn):
        self._fast_handlers[address] = fn

    # -- receiving --------------------------------------------------------------

    def _recv_loop(self):
        while not self._stop_event.is_set():
            try:
                ready, _w, _x = select.select([self._sock], [], [], 0.05)
            except OSError:
                if self._stop_event.is_set():
                    break
                continue
            if not ready:
                continue
            self._drain_available(dispatch_fast=True)

    def _drain_available(self, dispatch_fast):
        """Read every datagram currently available without blocking."""
        while True:
            try:
                data, addr = self._sock.recvfrom(65535)
            except BlockingIOError:
                break
            except ConnectionResetError:
                continue
            except OSError as exc:
                if _is_connreset(exc):
                    continue
                logger.exception("OSC recv error")
                break
            self._note_client(addr)
            try:
                messages = codec.decode_packet(data)
            except codec.OscError:
                logger.exception("failed to decode OSC packet from %s", addr)
                continue
            for address, args in messages:
                if dispatch_fast:
                    fast_fn = self._fast_handlers.get(address)
                    if fast_fn is not None:
                        try:
                            fast_fn(address, args)
                        except Exception:
                            logger.exception(
                                "fast handler failed for %s", address)
                        continue
                self._queue.append((address, args, addr))

    def process(self):
        """Main-thread tick: read the socket (non-fast mode) and drain queue."""
        if not self._fast_apply:
            self._drain_available(dispatch_fast=False)
        while True:
            try:
                address, args, _sender_addr = self._queue.popleft()
            except IndexError:
                break
            handler = self._normal_handlers.get(address)
            if handler is None:
                if address not in self._unknown_logged:
                    self._unknown_logged.add(address)
                    logger.warning("no handler registered for %s", address)
                continue
            try:
                handler(address, args)
            except Exception:
                logger.exception("handler failed for %s", address)

    # -- sending --------------------------------------------------------------

    def send(self, address, args=()):
        target = self.client_addr
        if target is None:
            return False
        return self.send_to(target, address, args)

    def send_to(self, addr, address, args=()):
        try:
            data = codec.encode_message(address, args)
            self._sock.sendto(data, addr)
            return True
        except OSError:
            logger.exception("failed to send %s to %s", address, addr)
            return False
        except codec.OscError:
            logger.exception("failed to encode %s", address)
            return False

    def send_bundle(self, messages):
        """Send (address, args) tuples as one or more OSC bundles to the
        current client, chunked so each datagram stays under
        config.MAX_OSC_PAYLOAD_BYTES."""
        target = self.client_addr
        if target is None:
            return False
        try:
            encoded = [codec.encode_message(address, args)
                       for address, args in messages]
        except codec.OscError:
            logger.exception("failed to encode bundle message")
            return False
        if not encoded:
            return True
        ok = True
        for chunk in _chunk_messages(encoded, config.MAX_OSC_PAYLOAD_BYTES):
            packet = codec.encode_bundle(chunk)
            try:
                self._sock.sendto(packet, target)
            except OSError:
                logger.exception("failed to send bundle to %s", target)
                ok = False
        return ok

    def shutdown(self):
        if self._closed:
            return
        self._closed = True
        self._stop_event.set()
        if self._thread is not None and self._thread.is_alive():
            self._thread.join(timeout=1.0)
        try:
            self._sock.close()
        except OSError:
            pass
