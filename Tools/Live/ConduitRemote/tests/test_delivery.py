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


# -- send_json_list (browser M4: element-level chunking) ----------------------

def _browser_entries(count):
    return [[i, "Entry with a fairly long name %04d" % i, 0, 1]
            for i in range(count)]


def test_list_small_payload_is_single_message():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=9000)
    sender.send_json_list("/remote/browser/list", 1, {"p": 0}, "it",
                          _browser_entries(3), force=True)
    assert len(osc.messages) == 1
    seq, chunk_index, chunk_count, payload_json = osc.messages[0][1]
    assert (seq, chunk_index, chunk_count) == (1, 1, 1)
    payload = json.loads(payload_json)
    assert payload["p"] == 0
    assert payload["it"] == _browser_entries(3)


def test_list_oversized_payload_chunks_by_element():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=200)
    entries = _browser_entries(20)
    sender.send_json_list("/remote/browser/list", 7, {"p": 2}, "it",
                          entries, force=True)
    assert len(osc.messages) > 1
    counts = set()
    indices = []
    reassembled = []
    for address, args in osc.messages:
        assert address == "/remote/browser/list"
        seq, chunk_index, chunk_count, payload_json = args
        assert seq == 7
        counts.add(chunk_count)
        indices.append(chunk_index)
        payload = json.loads(payload_json)
        assert payload["p"] == 2            # Meta reist in JEDEM Chunk mit
        reassembled.extend(payload["it"])
        assert len(payload_json.encode("utf-8")) <= 200
    assert len(counts) == 1
    assert sorted(indices) == list(range(1, list(counts)[0] + 1))
    assert reassembled == entries           # Reihenfolge bleibt erhalten


def test_list_oversized_element_is_dropped_not_raised():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=120)
    entries = [[1, "ok", 0, 1], [2, "x" * 500, 0, 1], [3, "ok too", 0, 1]]
    sender.send_json_list("/remote/browser/list", 1, {"p": 5}, "it",
                          entries, force=True)
    reassembled = []
    for _, args in osc.messages:
        reassembled.extend(json.loads(args[3])["it"])
    assert [e[0] for e in reassembled] == [1, 3]


def test_list_all_elements_dropped_still_answers():
    osc = RecordingOsc()
    sender = Sender(osc, max_bytes=60)
    entries = [[1, "y" * 500, 0, 1]]
    sender.send_json_list("/remote/browser/list", 1, {"p": 9}, "it",
                          entries, force=True)
    assert len(osc.messages) == 1           # Antwort geht IMMER raus
    payload = json.loads(osc.messages[0][1][3])
    assert payload == {"p": 9, "it": []}
