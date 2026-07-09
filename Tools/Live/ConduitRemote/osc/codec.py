"""Minimal OSC 1.0 codec (encode/decode, messages + bundles).

Written from scratch for ConduitRemote (MIT-clean, no third-party code).
Supported argument types: i (int32), f (float32), s (string), b (blob),
T/F (bool), d (float64 decode only -> float).  That covers everything the
ConduitRemote protocol uses.

Python 3.7+ compatible (Live 11 ships 3.7, Live 12 ships 3.11).
"""

import struct

BUNDLE_PREFIX = b"#bundle\x00"


class OscError(Exception):
    pass


# --- helpers ------------------------------------------------------------------

def _pad(data):
    """Pad bytes to a multiple of 4 with NUL bytes."""
    remainder = len(data) % 4
    if remainder:
        data += b"\x00" * (4 - remainder)
    return data


def _encode_string(value):
    data = value.encode("utf-8") + b"\x00"
    return _pad(data)


def _decode_string(data, offset):
    end = data.find(b"\x00", offset)
    if end < 0:
        raise OscError("unterminated OSC string")
    value = data[offset:end].decode("utf-8", errors="replace")
    # consume padding: string length including NUL rounded up to multiple of 4
    length = end - offset + 1
    padded = (length + 3) & ~3
    return value, offset + padded


def _decode_blob(data, offset):
    if offset + 4 > len(data):
        raise OscError("truncated blob size")
    (size,) = struct.unpack(">i", data[offset:offset + 4])
    offset += 4
    if size < 0 or offset + size > len(data):
        raise OscError("truncated blob payload")
    value = data[offset:offset + size]
    padded = (size + 3) & ~3
    return value, offset + padded


# --- message ------------------------------------------------------------------

def encode_message(address, args=()):
    """Encode an OSC message. Returns bytes."""
    if not address.startswith("/"):
        raise OscError("OSC address must start with '/': %r" % address)
    typetags = ","
    payload = b""
    for arg in args:
        if isinstance(arg, bool):          # must come before int check
            typetags += "T" if arg else "F"
        elif isinstance(arg, int):
            typetags += "i"
            payload += struct.pack(">i", arg)
        elif isinstance(arg, float):
            typetags += "f"
            payload += struct.pack(">f", arg)
        elif isinstance(arg, str):
            typetags += "s"
            payload += _encode_string(arg)
        elif isinstance(arg, (bytes, bytearray)):
            typetags += "b"
            payload += struct.pack(">i", len(arg)) + _pad(bytes(arg))
        else:
            raise OscError("unsupported OSC argument type: %r" % type(arg))
    return _encode_string(address) + _encode_string(typetags) + payload


def decode_message(data):
    """Decode a single OSC message. Returns (address, [args])."""
    address, offset = _decode_string(data, 0)
    if not address.startswith("/"):
        raise OscError("invalid OSC address: %r" % address)
    if offset >= len(data):
        return address, []
    typetags, offset = _decode_string(data, offset)
    if not typetags.startswith(","):
        raise OscError("invalid OSC type tag string: %r" % typetags)
    args = []
    for tag in typetags[1:]:
        if tag == "i":
            (value,) = struct.unpack(">i", data[offset:offset + 4])
            offset += 4
        elif tag == "f":
            (value,) = struct.unpack(">f", data[offset:offset + 4])
            offset += 4
        elif tag == "d":
            (value,) = struct.unpack(">d", data[offset:offset + 8])
            offset += 8
        elif tag == "s" or tag == "S":
            value, offset = _decode_string(data, offset)
        elif tag == "b":
            value, offset = _decode_blob(data, offset)
        elif tag == "T":
            value = True
        elif tag == "F":
            value = False
        elif tag == "N":
            value = None
        else:
            raise OscError("unsupported OSC type tag: %r" % tag)
        args.append(value)
    return address, args


# --- bundle -------------------------------------------------------------------

def is_bundle(data):
    return data[:8] == BUNDLE_PREFIX


def encode_bundle(messages, timetag=1):
    """Encode messages (list of encoded message bytes) into one bundle.

    timetag 1 == 'immediately' per OSC spec.
    """
    out = BUNDLE_PREFIX + struct.pack(">Q", timetag)
    for msg in messages:
        out += struct.pack(">i", len(msg)) + msg
    return out


def decode_packet(data):
    """Decode a datagram into a flat list of (address, args) tuples.

    Handles nested bundles; timetags are ignored (applied immediately),
    which matches how every Live remote script treats them.
    """
    if is_bundle(data):
        results = []
        offset = 16  # prefix + timetag
        while offset < len(data):
            if offset + 4 > len(data):
                raise OscError("truncated bundle element size")
            (size,) = struct.unpack(">i", data[offset:offset + 4])
            offset += 4
            if size < 0 or offset + size > len(data):
                raise OscError("truncated bundle element")
            results.extend(decode_packet(data[offset:offset + size]))
            offset += size
        return results
    return [decode_message(data)]
