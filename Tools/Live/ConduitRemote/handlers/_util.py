"""Small shared helpers for command handlers: safe coercion + clamping.

Handlers must never raise on malformed/out-of-range input (log & ignore
instead), so every conversion here has a safe fallback.
"""


def as_float(value, default=0.0):
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def as_int(value, default=0):
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def as_bool(value):
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        return value.strip().lower() not in ("", "0", "false", "off", "no")
    try:
        return bool(int(value))
    except (TypeError, ValueError):
        return bool(value)


def clamp(value, lo, hi):
    if lo > hi:
        lo, hi = hi, lo
    return max(lo, min(hi, value))


def set_param(param, value):
    """Set a DeviceParameter-like object's .value, clamped to its range."""
    lo = getattr(param, "min", 0.0)
    hi = getattr(param, "max", 1.0)
    param.value = clamp(as_float(value, getattr(param, "value", 0.0)), lo, hi)
