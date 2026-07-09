"""ConduitRemote configuration.

All tunables in one place. Values chosen per ABLETON-REMOTE.md (July 2026).
"""

# --- Network -----------------------------------------------------------------
# Deliberately distinct from Conduit's own OSC (9000/9001) and from
# AbletonOSC's defaults (11000/11001) so everything can run in parallel.
LISTEN_PORT = 9010          # script listens here (commands from Conduit)
RESPONSE_PORT = 9011        # replies/pushes go to sender_ip:RESPONSE_PORT
BIND_HOST = "0.0.0.0"       # wired-LAN setup: Conduit runs on another machine

# --- Fast path ---------------------------------------------------------------
# When True, a dedicated receiver thread applies whitelisted pure-value
# writes (track volume/pan/send, device parameter value) IMMEDIATELY on
# arrival instead of waiting for Live's ~100 ms scheduler tick.  This is the
# single deliberate deviation from the AbletonOSC pattern and the reason
# faders feel direct instead of 10 Hz-steppy.  Technically the LOM is
# documented as main-thread-only; value-only writes are field-proven but if
# anything misbehaves, set this to False (everything then flows through the
# main-thread queue and still works, just coarser).
FAST_APPLY = True

# --- Rates & limits ----------------------------------------------------------
TICK_INTERVAL = 1           # Live scheduler ticks between manager ticks (~100 ms)
METER_TICK_DIVIDER = 1      # send meters every N manager ticks (~10 Hz)
MAX_OSC_PAYLOAD_BYTES = 9000  # keep UDP datagrams well under typical MTU limits
HEARTBEAT_TIMEOUT_TICKS = 60  # ~6 s without /remote/ping -> client considered gone

# --- Protocol ----------------------------------------------------------------
PROTOCOL_VERSION = 1
DOMAINS = ("transport", "tracks", "mixer", "session")
