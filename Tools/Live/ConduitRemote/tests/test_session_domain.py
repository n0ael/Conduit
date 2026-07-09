import pytest

from ConduitRemote.sync import stable_ids
from ConduitRemote.sync.session import SessionDomain
from ConduitRemote.tests.stub_live import Song, FakeSender, Clip


@pytest.fixture(autouse=True)
def _clear_registry():
    stable_ids.clear()
    yield
    stable_ids.clear()


def make_domain(num_tracks=2, num_scenes=3):
    song = Song(num_tracks=num_tracks, num_scenes=num_scenes, num_sends=1)
    sender = FakeSender()
    domain = SessionDomain(song, sender)
    return song, sender, domain


def test_snapshot_shape_empty_grid():
    song, sender, domain = make_domain(num_tracks=2, num_scenes=3)
    domain.on_subscribe()
    address, seq, payload = sender.last()
    assert address == "/remote/state/session/snapshot"

    assert payload["scene:sc:1"] == {"name": "Scene 1", "color": 0, "index": 0}
    assert payload["scene:sc:2"]["index"] == 1
    assert payload["scene:sc:3"]["index"] == 2

    assert payload["grid:tr:1"] == [None, None, None]
    assert payload["grid:tr:2"] == [None, None, None]


def test_clip_in_slot_reported_stopped_by_default():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=2)
    song.tracks[0].clip_slots[0].set_clip(Clip("Loop", color=5))
    domain.on_subscribe()
    payload = sender.last()[2]
    row = payload["grid:tr:1"]
    assert row[0] == {"name": "Loop", "color": 5, "state": "stopped"}
    assert row[1] is None


def test_clip_state_precedence_recording_beats_playing():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=1)
    clip = Clip("Loop")
    song.tracks[0].clip_slots[0].set_clip(clip)
    clip.is_playing = True
    clip.is_recording = True
    domain.on_subscribe()
    row = sender.last()[2]["grid:tr:1"]
    assert row[0]["state"] == "recording"


def test_clip_state_precedence_triggered_beats_playing():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=1)
    clip = Clip("Loop")
    song.tracks[0].clip_slots[0].set_clip(clip)
    clip.is_playing = True
    clip.is_triggered = True
    domain.on_subscribe()
    row = sender.last()[2]["grid:tr:1"]
    assert row[0]["state"] == "triggered"


def test_clip_state_playing():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=1)
    clip = Clip("Loop")
    song.tracks[0].clip_slots[0].set_clip(clip)
    clip.is_playing = True
    domain.on_subscribe()
    row = sender.last()[2]["grid:tr:1"]
    assert row[0]["state"] == "playing"


def test_clip_playback_change_diffs_only_that_track_row():
    song, sender, domain = make_domain(num_tracks=2, num_scenes=2)
    clip_a = Clip("A")
    clip_b = Clip("B")
    song.tracks[0].clip_slots[0].set_clip(clip_a)
    song.tracks[1].clip_slots[0].set_clip(clip_b)
    domain.on_subscribe()
    sender.clear()

    clip_a.is_playing = True
    domain.on_tick(1)

    address, seq, diff = sender.last()
    assert address == "/remote/state/session/diff"
    assert list(diff.keys()) == ["grid:tr:1"]
    assert diff["grid:tr:1"][0]["state"] == "playing"


def test_new_clip_created_in_slot_marks_dirty_and_binds_listeners():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=1)
    domain.on_subscribe()
    sender.clear()

    clip = Clip("New")
    song.tracks[0].clip_slots[0].set_clip(clip)
    domain.on_tick(1)

    diff = sender.last()[2]
    assert diff["grid:tr:1"][0] == {"name": "New", "color": 0, "state": "stopped"}

    # now that the clip is bound, changing its state should mark dirty too
    sender.clear()
    clip.is_playing = True
    domain.on_tick(1)
    assert sender.last()[2]["grid:tr:1"][0]["state"] == "playing"


def test_removed_clip_reverts_slot_to_none():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=1)
    clip = Clip("Gone")
    slot = song.tracks[0].clip_slots[0]
    slot.set_clip(clip)
    domain.on_subscribe()
    sender.clear()

    slot.set_clip(None)
    domain.on_tick(1)

    diff = sender.last()[2]
    assert diff["grid:tr:1"][0] is None


def test_scene_rename_diffs_only_scene_key():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=2)
    domain.on_subscribe()
    sender.clear()

    song.scenes[0].name = "Intro"
    domain.on_tick(1)

    diff = sender.last()[2]
    assert list(diff.keys()) == ["scene:sc:1"]
    assert diff["scene:sc:1"]["name"] == "Intro"


def test_removing_scene_diffs_none():
    song, sender, domain = make_domain(num_tracks=1, num_scenes=2)
    domain.on_subscribe()
    sender.clear()

    song.scenes.pop(0)
    song.notify("scenes")
    domain.on_tick(1)

    diff = sender.last()[2]
    assert diff["scene:sc:1"] is None


def test_removing_track_diffs_grid_none():
    song, sender, domain = make_domain(num_tracks=2, num_scenes=2)
    domain.on_subscribe()
    sender.clear()

    song.tracks.pop(0)
    song.notify("tracks")
    domain.on_tick(1)

    diff = sender.last()[2]
    assert diff["grid:tr:1"] is None


def test_unsubscribed_emits_nothing():
    song, sender, domain = make_domain()
    domain.on_subscribe()
    domain.on_unsubscribe()
    sender.clear()

    song.scenes[0].name = "X"
    domain.on_tick(1)

    assert sender.sent == []


def test_detach_removes_listeners():
    song, sender, domain = make_domain()
    domain.on_subscribe()
    domain.detach()
    sender.clear()
    domain._dirty = False
    domain.subscribed = True

    song.scenes[0].name = "Y"
    domain.on_tick(1)

    assert sender.sent == []
