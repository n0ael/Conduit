"""/live/return, /live/master, /live/clip_slot, /live/scene handlers."""

import logging

from . import _util

logger = logging.getLogger(__name__)


def _return_track(ctx, index):
    song = ctx.song
    if song is None:
        return None
    returns = song.return_tracks
    if 0 <= index < len(returns):
        return returns[index]
    return None


def _return_volume(ctx, args):
    if len(args) < 2:
        return
    index = _util.as_int(args[0], -1)
    track = _return_track(ctx, index)
    if track is None:
        logger.debug("return volume: index out of range %r", args[0])
        return
    _util.set_param(track.mixer_device.volume, args[1])


def _return_panning(ctx, args):
    if len(args) < 2:
        return
    index = _util.as_int(args[0], -1)
    track = _return_track(ctx, index)
    if track is None:
        logger.debug("return panning: index out of range %r", args[0])
        return
    _util.set_param(track.mixer_device.panning, args[1])


def _master_volume(ctx, args):
    if len(args) < 1:
        return
    song = ctx.song
    if song is None:
        return
    _util.set_param(song.master_track.mixer_device.volume, args[0])


def _master_panning(ctx, args):
    if len(args) < 1:
        return
    song = ctx.song
    if song is None:
        return
    _util.set_param(song.master_track.mixer_device.panning, args[0])


def _clip_slot_fire(ctx, args):
    if len(args) < 2:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("clip_slot fire: unknown track %r", args[0])
        return
    index = _util.as_int(args[1], -1)
    if not (0 <= index < len(track.clip_slots)):
        logger.debug("clip_slot fire: index out of range %r", args[1])
        return
    track.clip_slots[index].fire()


def _clip_slot_stop(ctx, args):
    if len(args) < 2:
        return
    track = ctx.resolve_track(args[0])
    if track is None:
        logger.debug("clip_slot stop: unknown track %r", args[0])
        return
    index = _util.as_int(args[1], -1)
    if not (0 <= index < len(track.clip_slots)):
        logger.debug("clip_slot stop: index out of range %r", args[1])
        return
    track.clip_slots[index].stop()


def _scene_fire(ctx, args):
    if len(args) < 1:
        return
    song = ctx.song
    if song is None:
        return
    index = _util.as_int(args[0], -1)
    if not (0 <= index < len(song.scenes)):
        logger.debug("scene fire: index out of range %r", args[0])
        return
    song.scenes[index].fire()


def register_all(registry):
    registry.register("/live/return/set/volume", _return_volume)
    registry.register("/live/return/set/panning", _return_panning)
    registry.register("/live/master/set/volume", _master_volume)
    registry.register("/live/master/set/panning", _master_panning)
    registry.register("/live/clip_slot/fire", _clip_slot_fire)
    registry.register("/live/clip_slot/stop", _clip_slot_stop)
    registry.register("/live/scene/fire", _scene_fire)
