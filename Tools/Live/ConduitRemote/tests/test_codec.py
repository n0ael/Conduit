import math
import struct

import pytest

from ConduitRemote.osc import codec


def roundtrip(address, args=()):
    data = codec.encode_message(address, args)
    assert len(data) % 4 == 0
    decoded_address, decoded_args = codec.decode_message(data)
    assert decoded_address == address
    return decoded_args


def test_empty_message():
    assert roundtrip("/remote/ping") == []


def test_int_float_string():
    args = roundtrip("/live/track/set/volume", [3, 0.5, "hello"])
    assert args[0] == 3
    assert args[1] == pytest.approx(0.5)
    assert args[2] == "hello"


def test_bools_roundtrip():
    assert roundtrip("/x", [True, False]) == [True, False]


def test_unicode_string():
    assert roundtrip("/x", ["Grüße € 🎛"]) == ["Grüße € 🎛"]


def test_string_padding_boundaries():
    # lengths that land on every padding case (incl. exact multiple of 4)
    for n in range(1, 12):
        s = "a" * n
        assert roundtrip("/x", [s]) == [s]


def test_blob_roundtrip():
    blob = bytes(range(7))
    (out,) = roundtrip("/x", [blob])
    assert out == blob


def test_float_precision():
    (v,) = roundtrip("/x", [1.0 / 3.0])
    assert v == pytest.approx(1.0 / 3.0, abs=1e-7)


def test_bad_address_raises():
    with pytest.raises(codec.OscError):
        codec.encode_message("no_slash")


def test_double_decodes():
    # hand-build a message with a double ('d') argument
    data = codec._encode_string("/x") + codec._encode_string(",d")
    data += struct.pack(">d", math.pi)
    _, args = codec.decode_message(data)
    assert args[0] == pytest.approx(math.pi)


def test_bundle_roundtrip():
    msgs = [
        codec.encode_message("/a", [1]),
        codec.encode_message("/b", [2.5, "x"]),
    ]
    packet = codec.encode_bundle(msgs)
    assert codec.is_bundle(packet)
    decoded = codec.decode_packet(packet)
    assert decoded[0][0] == "/a"
    assert decoded[0][1] == [1]
    assert decoded[1][0] == "/b"
    assert decoded[1][1][1] == "x"


def test_nested_bundle():
    inner = codec.encode_bundle([codec.encode_message("/deep", ["ok"])])
    outer = codec.encode_bundle([codec.encode_message("/top"), inner])
    decoded = codec.decode_packet(outer)
    assert [d[0] for d in decoded] == ["/top", "/deep"]


def test_plain_message_via_decode_packet():
    data = codec.encode_message("/plain", [7])
    assert codec.decode_packet(data) == [("/plain", [7])]


def test_truncated_bundle_raises():
    packet = codec.encode_bundle([codec.encode_message("/a", [1])])
    with pytest.raises(codec.OscError):
        codec.decode_packet(packet[:-3])
