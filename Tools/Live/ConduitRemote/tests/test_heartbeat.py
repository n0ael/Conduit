from ConduitRemote.heartbeat import Heartbeat


def test_starts_not_alive():
    hb = Heartbeat(timeout_ticks=10)
    assert hb.alive is False


def test_alive_after_ping():
    hb = Heartbeat(timeout_ticks=10)
    hb.note_ping(0)
    assert hb.alive is True
    assert hb.check(0) is True
    assert hb.check(10) is True  # exactly at the boundary: still alive


def test_timeout_flips_alive_false():
    hb = Heartbeat(timeout_ticks=10)
    hb.note_ping(0)
    assert hb.check(11) is False
    assert hb.alive is False


def test_on_timeout_fires_once_per_transition():
    calls = []
    hb = Heartbeat(timeout_ticks=10, on_timeout=lambda: calls.append(1))
    hb.note_ping(0)
    hb.check(5)
    assert calls == []
    hb.check(11)
    assert calls == [1]
    # still timed out on subsequent checks -> no further calls
    hb.check(12)
    hb.check(20)
    assert calls == [1]


def test_recovers_after_new_ping():
    calls = []
    hb = Heartbeat(timeout_ticks=10, on_timeout=lambda: calls.append(1))
    hb.note_ping(0)
    hb.check(11)
    assert calls == [1]
    hb.note_ping(11)
    assert hb.check(11) is True
    assert hb.alive is True
    # a second timeout after recovery fires on_timeout again (new transition)
    hb.check(25)
    assert calls == [1, 1]


def test_never_pinged_check_does_not_fire_timeout():
    calls = []
    hb = Heartbeat(timeout_ticks=10, on_timeout=lambda: calls.append(1))
    assert hb.check(100) is False
    assert calls == []


def test_on_timeout_exception_is_swallowed():
    def boom():
        raise RuntimeError("boom")

    hb = Heartbeat(timeout_ticks=1, on_timeout=boom)
    hb.note_ping(0)
    # must not raise even though the callback blows up
    assert hb.check(5) is False
