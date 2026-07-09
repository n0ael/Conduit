"""Fast-Path v2 + Domain-Poll-Fallback (Feldtest 09.07.2026).

Hintergrund: Lives embedded Python schedult Background-Threads nur im
~100-ms-Tick (GIL) - der fruehere RX-Thread brachte nichts (111-ms-Stufen
in der Automations-Messung des Users). Der Fast-Path laeuft jetzt ueber
OscServer.pump() aus einem Live.Base.Timer; ohne Live-Modul (Tests) faellt
die Factory auf None zurueck (Tick-Rate).
"""

from ConduitRemote.manager import _default_timer_factory
from ConduitRemote.sync.base import Domain
from ConduitRemote.tests.stub_live import FakeSender


def test_default_timer_factory_returns_none_without_live():
    # In der Testumgebung gibt es kein Live-Modul -> Fallback (Tick-Rate)
    assert _default_timer_factory(lambda: None) is None


class _CrashingAttachDomain(Domain):
    """Simuliert Live 12.4b: Listener-Binden wirft (Boost ArgumentError)."""

    NAME = "crashy"

    def __init__(self, sender):
        Domain.__init__(self, None, sender)
        self.value = 1

    def on_attach(self):
        raise RuntimeError("Live 12.4b: ArgumentError beim Listener-Binden")

    def collect(self):
        return {"value": self.value}


def test_domain_attach_crash_falls_back_to_polling():
    sender = FakeSender()
    domain = _CrashingAttachDomain(sender)

    domain.on_subscribe()          # darf nicht werfen; Snapshot geht raus
    assert sender.last()[0] == "/remote/state/crashy/snapshot"

    sender.clear()
    domain.on_tick(1)              # unveraendert -> kein Diff
    assert sender.sent == []

    domain.value = 2               # OHNE Listener -> Polling muss es finden
    domain.on_tick(2)
    assert sender.last()[0] == "/remote/state/crashy/diff"
    assert sender.last()[2] == {"value": 2}


def test_detach_crash_is_swallowed():
    class _CrashingDetachDomain(_CrashingAttachDomain):
        NAME = "crashy_detach"

        def on_attach(self):
            pass

        def on_detach(self):
            raise RuntimeError("LOM-Objekt beim Teardown schon tot")

    domain = _CrashingDetachDomain(FakeSender())
    domain.on_subscribe()
    domain.detach()                # must not raise
    assert domain._attached is False
