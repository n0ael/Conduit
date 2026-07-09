"""End-to-end integration: real Manager + real domains + real UDP sockets.

Covers the full M1a loop:
  client subscribes -> snapshot arrives on the response port ->
  client moves a fader (fast path via Live.Base.Timer-pump, stable-id
  addressing) -> value is applied without a manager tick ->
  next manager tick emits a mixer diff containing exactly that change.
"""

import json
import socket
import time

import pytest

from ConduitRemote import config
from ConduitRemote.sync import stable_ids
from ConduitRemote.osc import codec
from ConduitRemote.tests.stub_live import Song


class FakeCInstance(object):
    def song(self):
        raise AssertionError("song is injected explicitly in tests")


class FakeTimer(object):
    """Live.Base.Timer-Ersatz: der Test feuert fire() von Hand."""

    def __init__(self, callback):
        self.callback = callback

    def start(self):
        pass

    def stop(self):
        pass

    def fire(self):
        self.callback()


@pytest.fixture
def rig(monkeypatch):
    """Manager on an OS-assigned port + response listener socket."""
    stable_ids.clear()

    response_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    response_sock.bind(("127.0.0.1", 0))
    response_sock.settimeout(0.05)
    response_port = response_sock.getsockname()[1]

    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    probe.bind(("127.0.0.1", 0))
    listen_port = probe.getsockname()[1]
    probe.close()

    monkeypatch.setattr(config, "BIND_HOST", "127.0.0.1")
    monkeypatch.setattr(config, "LISTEN_PORT", listen_port)
    monkeypatch.setattr(config, "RESPONSE_PORT", response_port)
    monkeypatch.setattr(config, "FAST_APPLY", True)

    from ConduitRemote.manager import Manager
    song = Song(num_tracks=3, num_scenes=4)

    def timer_factory(callback):
        timer = FakeTimer(callback)
        timer.start()
        return timer

    manager = Manager(FakeCInstance(), song=song, timer_factory=timer_factory)

    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    yield manager, song, client, response_sock, listen_port

    client.close()
    response_sock.close()
    manager.disconnect()


def send_cmd(client, listen_port, address, args=()):
    client.sendto(codec.encode_message(address, args),
                  ("127.0.0.1", listen_port))


def recv_state(response_sock, want_address, manager):
    """Pump-tick-and-receive until want_address arrives (im Timer-Modus
    liest NUR der Pump den Socket — wie in Live selbst)."""
    deadline = time.time() + 3.0
    while time.time() < deadline:
        manager._fast_timer.fire()
        manager.tick()
        try:
            data, _ = response_sock.recvfrom(65536)
        except socket.timeout:
            continue
        for address, args in codec.decode_packet(data):
            if address == want_address:
                seq, chunk_i, chunk_n, payload = args[0], args[1], args[2], args[3]
                assert chunk_i == 1 and chunk_n == 1
                return seq, json.loads(payload)
    raise AssertionError("no %s received" % want_address)


def test_full_m1a_loop(rig):
    manager, song, client, response_sock, listen_port = rig

    # 1) subscribe to mixer -> snapshot arrives
    send_cmd(client, listen_port, "/remote/state/mixer/subscribe")
    seq, snapshot = recv_state(response_sock, "/remote/state/mixer/snapshot",
                               manager)
    assert len(snapshot) == 3 + 2 + 1   # tracks + returns + master
    track_id = stable_ids.get_id(song.tracks[1], stable_ids.TRACK_PREFIX)
    assert snapshot[track_id]["vol"] == pytest.approx(0.85)

    # 2) fader move via FAST PATH with stable-id addressing: applied by the
    #    timer pump (Live.Base.Timer), no manager.tick() in between.
    send_cmd(client, listen_port, "/live/track/set/volume", [track_id, 0.42])
    deadline = time.time() + 1.0
    while time.time() < deadline:
        manager._fast_timer.fire()
        if abs(song.tracks[1].mixer_device.volume.value - 0.42) < 1e-6:
            break
        time.sleep(0.005)
    assert song.tracks[1].mixer_device.volume.value == pytest.approx(0.42), \
        "fast path did not apply the value from the timer pump"

    # 3) next tick -> diff with exactly the changed track
    seq2, diff = recv_state(response_sock, "/remote/state/mixer/diff", manager)
    assert seq2 > seq
    assert list(diff.keys()) == [track_id]
    assert diff[track_id]["vol"] == pytest.approx(0.42)


def test_transport_and_session_loop(rig):
    manager, song, client, response_sock, listen_port = rig

    send_cmd(client, listen_port, "/remote/state/transport/subscribe")
    send_cmd(client, listen_port, "/remote/state/session/subscribe")
    _, transport = recv_state(response_sock,
                              "/remote/state/transport/snapshot", manager)
    assert transport["is_playing"] is False
    recv_state(response_sock, "/remote/state/session/snapshot", manager)

    # start playback + fire a scene through the command path
    send_cmd(client, listen_port, "/live/song/start_playing")
    send_cmd(client, listen_port, "/live/scene/fire", [2])
    deadline = time.time() + 2.0
    while time.time() < deadline and not (
            song.is_playing and song.scenes[2].__dict__.get("fired")):
        manager._fast_timer.fire()
        manager.tick()
        time.sleep(0.01)
    assert song.is_playing is True
    assert song.scenes[2].__dict__.get("fired") is True

    _, diff = recv_state(response_sock, "/remote/state/transport/diff", manager)
    assert diff.get("is_playing") is True


def test_ping_pong_and_heartbeat(rig):
    manager, song, client, response_sock, listen_port = rig

    send_cmd(client, listen_port, "/remote/ping")
    deadline = time.time() + 3.0
    got_pong = False
    while time.time() < deadline and not got_pong:
        manager._fast_timer.fire()
        manager.tick()
        try:
            data, _ = response_sock.recvfrom(65536)
        except socket.timeout:
            continue
        for address, args in codec.decode_packet(data):
            if address == "/remote/pong":
                assert args[0] == config.PROTOCOL_VERSION
                got_pong = True
    assert got_pong
