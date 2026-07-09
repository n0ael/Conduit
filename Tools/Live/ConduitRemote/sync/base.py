"""Domain base class: subscribe -> snapshot -> diffs.

Architecture pattern (not code) follows the domain-sync idea documented in
ABLETON-REMOTE.md par.2: each domain owns one slice of Live state, can produce a
full snapshot dict, and on every manager tick emits a diff against what it
last sent.  Transport of snapshots/diffs is JSON over OSC:

    /remote/state/{name}/snapshot  [seq:int, json:str]
    /remote/state/{name}/diff      [seq:int, json:str]

Diff semantics: {key: value} for changed/added keys, {key: None} for removed
keys.  Keys are stable IDs (or fixed names for scalar domains), values are
JSON-serialisable dicts/scalars.  The client applies diffs onto its model;
a lost datagram is healed by the next full snapshot request (client detects
seq gaps and re-requests).

Subclasses implement:
    NAME            str, domain name
    collect()       -> dict  (current full state; called on main thread)
and may override on_attach()/on_detach() to add/remove Live listeners that
mark the domain dirty (cheap change hints; collect() stays the truth).
"""

import json


class Domain(object):
    NAME = "base"

    def __init__(self, live_song, sender):
        """live_song: Live song object (or stub).  sender: delivery.Sender."""
        self.song = live_song
        self.sender = sender
        self.subscribed = False
        self.seq = 0
        self._last = {}
        self._dirty = True   # first tick after subscribe sends everything
        self._attached = False

    # -- lifecycle --------------------------------------------------------

    def attach(self):
        if not self._attached:
            self.on_attach()
            self._attached = True

    def detach(self):
        if self._attached:
            self.on_detach()
            self._attached = False

    def on_attach(self):
        pass

    def on_detach(self):
        pass

    # -- change hints -----------------------------------------------------

    def mark_dirty(self):
        self._dirty = True

    # -- protocol entry points (called by Routes / Manager) ----------------

    def on_subscribe(self):
        self.subscribed = True
        self.attach()
        self.send_snapshot()

    def on_unsubscribe(self):
        self.subscribed = False

    def send_snapshot(self):
        state = self.collect()
        self._last = state
        self._dirty = False
        self.seq += 1
        # force=True: an explicitly re-requested snapshot must always go out,
        # even if the payload is identical to the last send (client heals
        # lost datagrams by re-requesting).
        self.sender.send_json(
            "/remote/state/%s/snapshot" % self.NAME, self.seq, state,
            force=True)

    def on_tick(self, tick_index):
        if not self.subscribed or not self._dirty:
            return
        state = self.collect()
        diff = compute_diff(self._last, state)
        self._last = state
        self._dirty = False
        if diff:
            self.seq += 1
            self.sender.send_json(
                "/remote/state/%s/diff" % self.NAME, self.seq, diff)

    # -- to implement ------------------------------------------------------

    def collect(self):
        raise NotImplementedError


def compute_diff(old, new):
    """Shallow diff of two dicts: changed/added keys -> new value,
    removed keys -> None.  Values compared via JSON-stable equality."""
    diff = {}
    for key, value in new.items():
        if key not in old or old[key] != value:
            diff[key] = value
    for key in old:
        if key not in new:
            diff[key] = None
    return diff


def to_json(obj):
    """Compact, key-sorted JSON (stable output => dedupe-friendly)."""
    return json.dumps(obj, separators=(",", ":"), sort_keys=True)
