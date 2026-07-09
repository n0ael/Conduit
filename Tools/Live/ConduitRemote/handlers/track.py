"""/live/track/* command handlers (AbletonOSC-compatible addresses).

track_ref (args[0] of every handler here) is resolved via
ctx.resolve_track(): an int index into song.tracks, or a stable-id string
once the sync layer's resolver is wired in.
"""

import logging

from . import _util

logger = logging.getLogger(__name__)


def _volume(ctx, args):
    if len(args) < 2:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("track volume: unknown track %r", args[0])
        return
    _util.set_param(track.mixer_device.volume, args[1])


def _panning(ctx, args):
    if len(args) < 2:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("track panning: unknown track %r", args[0])
        return
    _util.set_param(track.mixer_device.panning, args[1])


def _send(ctx, args):
    if len(args) < 3:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("track send: unknown track %r", args[0])
        return
    sends = track.mixer_device.sends
    index = _util.as_int(args[1], -1)
    if not (0 <= index < len(sends)):
        logger.debug("track send: index out of range %r", args[1])
        return
    _util.set_param(sends[index], args[2])


def _set_bool_prop(ctx, args, prop):
    if len(args) < 2:
        return None
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("track %s: unknown track %r", prop, args[0])
        return None
    setattr(track, prop, _util.as_bool(args[1]))
    return track


def _mute(ctx, args):
    _set_bool_prop(ctx, args, "mute")


def _solo(ctx, args):
    _set_bool_prop(ctx, args, "solo")


def _arm(ctx, args):
    if len(args) < 2:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("track arm: unknown track %r", args[0])
        return
    if not getattr(track, "can_be_armed", True):
        logger.debug("track arm: track cannot be armed")
        return
    track.arm = _util.as_bool(args[1])


def _stop_all_clips(ctx, args):
    if len(args) < 1:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("stop_all_clips: unknown track %r", args[0])
        return
    track.stop_all_clips()


def register_all(registry):
    registry.register("/live/track/set/volume", _volume)
    registry.register("/live/track/set/panning", _panning)
    registry.register("/live/track/set/send", _send)
    registry.register("/live/track/set/mute", _mute)
    registry.register("/live/track/set/solo", _solo)
    registry.register("/live/track/set/arm", _arm)
    registry.register("/live/track/stop_all_clips", _stop_all_clips)
