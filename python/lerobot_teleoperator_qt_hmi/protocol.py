"""Wire protocol shared by the Qt HMI and the Python side.

Messages are single-frame JSON objects. JSON (rather than msgpack) is used
deliberately so the C++/Qt client needs nothing beyond QJsonDocument.

Two logical channels (see bridge.py for the socket topology):
  Qt  -> Python : action and command frames
  Python -> Qt  : observation, features and status frames

This module has no third-party dependencies beyond the standard library, so
it can be imported and exercised without lerobot installed.
"""
from __future__ import annotations

import json
import time
from typing import Any

PROTOCOL_VERSION = 1

# Message types (the "type" field on every frame).
T_ACTION = "action"            # Qt  -> Python: goal positions
T_COMMAND = "command"          # Qt  -> Python: control-plane verb
T_OBSERVATION = "observation"  # Python -> Qt: joint state
T_FEATURES = "features"        # Python -> Qt: joint names + ranges (UI build)
T_STATUS = "status"            # Python -> Qt: connection / calibration / errors

# Control-plane command verbs.
CMD_ENABLE = "enable"
CMD_DISABLE = "disable"
CMD_HOME = "home"
CMD_ESTOP = "estop"


def now() -> float:
    return time.time()


class _Encoder(json.JSONEncoder):
    """Tolerates numpy scalars/arrays so observation dicts serialize cleanly."""

    def default(self, o: Any):
        if hasattr(o, "item"):
            try:
                return o.item()
            except Exception:
                pass
        if hasattr(o, "tolist"):
            return o.tolist()
        return super().default(o)


def encode(msg: dict[str, Any]) -> bytes:
    msg.setdefault("v", PROTOCOL_VERSION)
    msg.setdefault("ts", now())
    return json.dumps(msg, cls=_Encoder, separators=(",", ":")).encode("utf-8")


def decode(raw: bytes) -> dict[str, Any]:
    return json.loads(raw.decode("utf-8"))


# --- frame builders --------------------------------------------------------

def action_msg(action: dict[str, float], seq: int) -> dict[str, Any]:
    return {"type": T_ACTION, "seq": seq, "action": action}


def command_msg(verb: str, seq: int) -> dict[str, Any]:
    return {"type": T_COMMAND, "seq": seq, "command": verb}


def observation_msg(state: dict[str, float], seq: int, connected: bool) -> dict[str, Any]:
    return {"type": T_OBSERVATION, "seq": seq, "connected": bool(connected), "state": state}


def features_msg(joints, ranges: dict[str, tuple]) -> dict[str, Any]:
    return {
        "type": T_FEATURES,
        "joints": list(joints),
        "ranges": {k: list(v) for k, v in ranges.items()},
    }


def status_msg(connected: bool, calibrated: bool = True, message: str = "") -> dict[str, Any]:
    return {
        "type": T_STATUS,
        "connected": bool(connected),
        "calibrated": bool(calibrated),
        "message": message,
    }
