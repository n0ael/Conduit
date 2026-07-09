import json

from ConduitRemote.sync.delivery import Sender


class RecordingOsc(object):
    def __init__(self):
        self.messages = []   # list of (address, args)

    def __call__(self, address, args):
        self.messages.append((address, list(args)))


def test_single_message_fits():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=9000)
    sender.send_json("/remote/state/transport/snapshot", 1, {"tempo": 120.0})
    assert len(osc.messages) == 1
    address, args = osc.messages[0]
    assert address == "/remote/state/transport/snapshot"
    seq, chunk_index, chunk_count, payload_json = args
    assert seq == 1
    assert chunk_index == 1
    assert chunk_count == 1
    assert json.loads(payload_json) == {"tempo": 120.0}


def test_chunking_of_oversized_payload():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=120)
    payload = {
        "tr:%d" % i: {"name": "Track with a fairly long name %d" % i, "color": i}
        for i in range(10)
    }
    sender.send_json("/remote/state/tracks/snapshot", 1, payload)
    assert len(osc.messages) > 1
    # every message shares seq and chunk_count, chunk_index is 1..n unique
    seqs = set()
    counts = set()
    indices = []
    reassembled = {}
    for address, args in osc.messages:
        assert address == "/remote/state/tracks/snapshot"
        seq, chunk_index, chunk_count, payload_json = args
        seqs.add(seq)
        counts.add(chunk_count)
        indices.append(chunk_index)
        chunk_dict = json.loads(payload_json)
        reassembled.update(chunk_dict)
        assert len(payload_json.encode("utf-8")) <= 120
    assert seqs == {1}
    assert len(counts) == 1
    assert sorted(indices) == list(range(1, list(counts)[0] + 1))
    assert reassembled == payload


def test_oversized_single_key_is_dropped_not_raised():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=40)
    payload = {
        "a": "short",
        "huge": "x" * 500,
    }
    # must not raise
    sender.send_json("/remote/state/x/snapshot", 1, payload)
    reassembled = {}
    for address, args in osc.messages:
        chunk_dict = json.loads(args[3])
        reassembled.update(chunk_dict)
    assert reassembled == {"a": "short"}
    assert "huge" not in reassembled


def test_dedupe_skips_identical_resend():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=9000)
    sender.send_json("/remote/state/transport/diff", 1, {"tempo": 120.0})
    sender.send_json("/remote/state/transport/diff", 2, {"tempo": 120.0})
    assert len(osc.messages) == 1


def test_dedupe_allows_changed_payload():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=9000)
    sender.send_json("/remote/state/transport/diff", 1, {"tempo": 120.0})
    sender.send_json("/remote/state/transport/diff", 2, {"tempo": 121.0})
    assert len(osc.messages) == 2


def test_force_bypasses_dedupe():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=9000)
    sender.send_json("/remote/state/transport/snapshot", 1, {"tempo": 120.0})
    sender.send_json("/remote/state/transport/snapshot", 2, {"tempo": 120.0},
                      force=True)
    assert len(osc.messages) == 2


def test_dedupe_is_per_address():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=9000)
    sender.send_json("/remote/state/transport/diff", 1, {"tempo": 120.0})
    sender.send_json("/remote/state/mixer/diff", 2, {"tempo": 120.0})
    assert len(osc.messages) == 2
