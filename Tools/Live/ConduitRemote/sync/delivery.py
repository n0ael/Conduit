"""Delivery: turns a Domain's payload dict into one or more OSC messages.

Wire format (per address):
    [seq:int, chunk_index:int (1-based), chunk_count:int, json:str]

A payload that serialises small enough is sent as a single message with
chunk_index=1, chunk_count=1.  A payload too large for one UDP-safe message
is split by TOP-LEVEL KEY into several sub-dicts, each individually small
enough, and sent as chunk_index=1..chunk_count.  The client reassembles by
merging all chunks sharing the same seq before applying them as one diff/
snapshot.  A single key/value pair that alone is too large to ever fit is
logged and dropped -- send_json() never raises for an oversized payload.

Dedupe: Sender remembers, per OSC address, a crc32 digest of the last FULL
payload's JSON it was asked to send (regardless of whether that send ended
up chunked).  If the next call's full payload hashes the same, the call is
a no-op (nothing goes out, not even a single chunk) -- this collapses
"nothing changed" ticks. Pass force=True to bypass this (used for
explicitly-requested snapshots, which must always go out).
"""

import logging
import zlib

from .base import to_json
from .. import config

logger = logging.getLogger(__name__)


class Sender(object):
    def __init__(self, osc_send, max_bytes=config.MAX_OSC_PAYLOAD_BYTES):
        """osc_send: callable(address, args_list) that actually puts an OSC
        message on the wire (or, in tests, stub_live.FakeSender-like capture)."""
        self.osc_send = osc_send
        self.max_bytes = max_bytes
        self._last_digest = {}   # address -> int crc32

    def send_json(self, address, seq, payload_dict, force=False):
        full_json = to_json(payload_dict)
        full_encoded = full_json.encode("utf-8")
        digest = zlib.crc32(full_encoded) & 0xffffffff

        if not force and self._last_digest.get(address) == digest:
            return   # identical to last full send for this address: skip

        self._last_digest[address] = digest

        if len(full_encoded) <= self.max_bytes:
            self.osc_send(address, [seq, 1, 1, full_json])
            return

        chunks = self._split(payload_dict)
        n = len(chunks)
        if n == 0:
            # every key was individually oversized and got dropped
            logger.warning(
                "delivery: %s payload had no key that fit max_bytes=%d; "
                "nothing sent", address, self.max_bytes)
            return
        for i, chunk in enumerate(chunks, start=1):
            chunk_json = to_json(chunk)
            self.osc_send(address, [seq, i, n, chunk_json])

    # -- internals ----------------------------------------------------------

    def _fits(self, obj):
        return len(to_json(obj).encode("utf-8")) <= self.max_bytes

    def _split(self, payload_dict):
        """Greedy bin-pack top-level keys into as few <=max_bytes chunks as
        possible, in sorted-key order (deterministic). A key whose own
        single-key dict does not fit is dropped (logged), never raised."""
        chunks = []
        current = {}
        for key in sorted(payload_dict.keys()):
            value = payload_dict[key]
            candidate = dict(current)
            candidate[key] = value
            if self._fits(candidate):
                current = candidate
                continue
            # candidate (current + key) doesn't fit -- flush current first
            if current:
                chunks.append(current)
                current = {}
            if self._fits({key: value}):
                current = {key: value}
            else:
                logger.warning(
                    "delivery: dropping oversized key %r (exceeds "
                    "max_bytes=%d alone)", key, self.max_bytes)
        if current:
            chunks.append(current)
        return chunks
