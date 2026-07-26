"""Server-side ZeroMQ transport for the Qt HMI.

Topology (the Python side is the stable server and binds both sockets; the Qt
HMI is the client and connects to both):

    action_bind     PULL  <--  Qt PUSH   (goal positions, control commands)
    telemetry_bind  PUB    -->  Qt SUB   (observations, features, status)

ZeroMQ sockets are not thread-safe. A HmiBridge and all of its sockets must be
created and used on a single thread. Do not share one across threads.

No lerobot dependency here, so the transport can be tested on its own.
"""
from __future__ import annotations

from typing import Any, Optional

import zmq

from . import protocol


class HmiBridge:
    def __init__(
        self,
        action_bind: str = "tcp://*:5556",
        telemetry_bind: str = "tcp://*:5557",
        rcvhwm: int = 16,
    ):
        self._ctx = zmq.Context.instance()

        # Inbound actions/commands from the HMI. A small receive high-water mark
        # plus draining to the latest frame each cycle gives us latest-wins
        # behaviour without blocking the HMI.
        self._pull = self._ctx.socket(zmq.PULL)
        self._pull.setsockopt(zmq.RCVHWM, rcvhwm)
        self._pull.bind(action_bind)

        # Outbound telemetry to the HMI.
        self._pub = self._ctx.socket(zmq.PUB)
        self._pub.bind(telemetry_bind)

    def poll_inbound(self) -> tuple[Optional[dict[str, float]], list[str]]:
        """Drain the action socket without blocking.

        Returns the newest action frame seen this cycle (or None) plus every
        control command received, in arrival order.
        """
        latest_action: Optional[dict[str, float]] = None
        commands: list[str] = []
        while True:
            try:
                raw = self._pull.recv(flags=zmq.NOBLOCK)
            except zmq.Again:
                break
            try:
                msg = protocol.decode(raw)
            except Exception:
                continue
            mtype = msg.get("type")
            if mtype == protocol.T_ACTION:
                latest_action = msg.get("action") or {}
            elif mtype == protocol.T_COMMAND:
                verb = msg.get("command")
                if verb:
                    commands.append(verb)
        return latest_action, commands

    def publish(self, msg: dict[str, Any]) -> None:
        self._pub.send(protocol.encode(msg))

    def publish_observation(self, state: dict[str, float], seq: int, connected: bool = True) -> None:
        self.publish(protocol.observation_msg(state, seq, connected))

    def publish_features(self, joints, ranges: dict[str, tuple]) -> None:
        self.publish(protocol.features_msg(joints, ranges))

    def publish_status(self, connected: bool, calibrated: bool = True, message: str = "") -> None:
        self.publish(protocol.status_msg(connected, calibrated, message))

    def close(self) -> None:
        for sock in (self._pull, self._pub):
            try:
                sock.close(0)
            except Exception:
                pass
