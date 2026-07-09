"""ConduitRemote - OSC remote script for Ableton Live 11/12.

Companion to Conduit's Ableton-Remote page (Omega).  See README.md.
Command schema follows AbletonOSC (MIT, (c) Daniel Jones) conventions;
sync layer and fast path are original work.
"""


def create_instance(c_instance):
    # Lazy import: keeps the package importable outside Live (unit tests).
    from .manager import Manager
    return Manager(c_instance)
