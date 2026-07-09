import socket
import time

import pytest

from ConduitRemote.osc import codec
from ConduitRemote.osc.server import OscServer

HOST = "127.0.0.1"


def poll_until(predicate, timeout=0.5, interval=0.01):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(interval)
    return predicate()


def make_client_socket():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, 0))
    sock.settimeout(1.0)
    return sock


@pytest.fixture
def response_listener():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, 0))
    sock.settimeout(1.0)
    yield sock
    sock.close()


def make_server(response_port, fast_apply=True):
    server = OscServer(HOST, 0, response_port, fast_apply=fast_apply)
    return server


# -- fast path ----------------------------------------------------------------

def test_fast_whitelisted_address_applied_without_process():
    server = make_server(response_port=1, fast_apply=True)
    try:
        calls = []
        server.set_fast_handler("/live/track/set/volume",
                                 lambda addr, args: calls.append(args))

        client = make_client_socket()
        data = codec.encode_message("/live/track/set/volume", [0, 0.5])
        client.sendto(data, server.getsockname())

        assert poll_until(lambda: len(calls) == 1, timeout=0.5)
        assert calls[0] == [0, 0.5]
        client.close()
    finally:
        server.shutdown()


def test_non_whitelisted_address_waits_for_process():
    server = make_server(response_port=1, fast_apply=True)
    try:
        calls = []
        server.add_handler("/live/track/set/mute",
                            lambda addr, args: calls.append(args))

        client = make_client_socket()
        data = codec.encode_message("/live/track/set/mute", [0, 1])
        client.sendto(data, server.getsockname())

        # give the RX thread time to pick it up into the queue - it must
        # NOT be dispatched without an explicit process() call.
        time.sleep(0.2)
        assert calls == []

        server.process()
        assert poll_until(lambda: len(calls) == 1, timeout=0.5)
        assert calls[0] == [0, 1]
        client.close()
    finally:
        server.shutdown()


def test_unknown_address_does_not_crash_process():
    server = make_server(response_port=1, fast_apply=True)
    try:
        client = make_client_socket()
        data = codec.encode_message("/live/nope/nope", [1])
        client.sendto(data, server.getsockname())
        time.sleep(0.1)
        server.process()  # must not raise
        client.close()
    finally:
        server.shutdown()


# -- non-fast (classic poll) mode ----------------------------------------------

def test_fast_apply_false_only_works_through_process():
    server = make_server(response_port=1, fast_apply=False)
    try:
        assert server._thread is None

        calls = []
        # Even a "fast-whitelisted-style" address only reaches the normal
        # registry in this mode - fast handlers are never consulted.
        server.set_fast_handler("/live/track/set/volume",
                                 lambda addr, args: calls.append(("fast", args)))
        server.add_handler("/live/track/set/volume",
                            lambda addr, args: calls.append(("normal", args)))

        client = make_client_socket()
        data = codec.encode_message("/live/track/set/volume", [0, 0.5])
        client.sendto(data, server.getsockname())

        time.sleep(0.1)
        assert calls == []  # nothing reads the socket without process()

        server.process()
        assert calls == [("normal", [0, 0.5])]
        client.close()
    finally:
        server.shutdown()


# -- client tracking ------------------------------------------------------------

def test_client_addr_learned_from_inbound_packet():
    server = make_server(response_port=1, fast_apply=True)
    try:
        assert server.client_addr is None
        client = make_client_socket()
        data = codec.encode_message("/remote/ping", [])
        client.sendto(data, server.getsockname())

        assert poll_until(lambda: server.client_addr is not None, timeout=0.5)
        ip, port = server.client_addr
        assert ip == HOST
        assert port == 1
        client.close()
    finally:
        server.shutdown()


# -- sending --------------------------------------------------------------------

def test_send_reaches_response_listener(response_listener):
    response_port = response_listener.getsockname()[1]
    server = make_server(response_port=response_port, fast_apply=True)
    try:
        client = make_client_socket()
        client.sendto(codec.encode_message("/remote/ping", []),
                      server.getsockname())
        assert poll_until(lambda: server.client_addr is not None, timeout=0.5)

        ok = server.send("/remote/pong", [1])
        assert ok is True

        data, _addr = response_listener.recvfrom(65535)
        address, args = codec.decode_message(data)
        assert address == "/remote/pong"
        assert args == [1]
        client.close()
    finally:
        server.shutdown()


def test_send_without_known_client_returns_false():
    server = make_server(response_port=1, fast_apply=True)
    try:
        assert server.client_addr is None
        assert server.send("/remote/pong", [1]) is False
    finally:
        server.shutdown()


def test_send_bundle_reaches_response_listener(response_listener):
    response_port = response_listener.getsockname()[1]
    server = make_server(response_port=response_port, fast_apply=True)
    try:
        client = make_client_socket()
        client.sendto(codec.encode_message("/remote/ping", []),
                      server.getsockname())
        assert poll_until(lambda: server.client_addr is not None, timeout=0.5)

        ok = server.send_bundle([
            ("/remote/state/a/diff", [1, "{}"]),
            ("/remote/state/b/diff", [2, "{}"]),
        ])
        assert ok is True

        data, _addr = response_listener.recvfrom(65535)
        messages = codec.decode_packet(data)
        assert ("/remote/state/a/diff", [1, "{}"]) in messages
        assert ("/remote/state/b/diff", [2, "{}"]) in messages
        client.close()
    finally:
        server.shutdown()


# -- ECONNRESET quirk ------------------------------------------------------------

class _FlakySocket(object):
    """Wraps a real socket, forcing the first recvfrom() to look like the
    Windows WSAECONNRESET quirk before behaving like a normal non-blocking
    socket with nothing pending."""

    def __init__(self, real_sock):
        self._real = real_sock
        self.calls = 0

    def recvfrom(self, bufsize):
        self.calls += 1
        if self.calls == 1:
            raise ConnectionResetError(10054, "connection reset")
        raise BlockingIOError()

    def __getattr__(self, name):
        return getattr(self._real, name)


def test_drain_available_ignores_connreset():
    server = make_server(response_port=1, fast_apply=False)
    real_sock = server._sock
    try:
        server._sock = _FlakySocket(real_sock)
        server._drain_available(dispatch_fast=False)  # must not raise
        assert server._sock.calls == 2
    finally:
        server._sock = real_sock
        server.shutdown()


# -- shutdown ---------------------------------------------------------------------

def test_shutdown_idempotent_and_releases_port():
    server = make_server(response_port=1, fast_apply=True)
    bound_port = server.getsockname()[1]
    server.shutdown()
    server.shutdown()  # must not raise

    # port must be free again
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    probe.bind((HOST, bound_port))
    probe.close()


def test_shutdown_stops_thread():
    server = make_server(response_port=1, fast_apply=True)
    thread = server._thread
    assert thread is not None
    assert thread.is_alive()
    server.shutdown()
    assert poll_until(lambda: not thread.is_alive(), timeout=1.0)
