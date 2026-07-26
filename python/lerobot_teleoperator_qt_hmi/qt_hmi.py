"""Qt HMI teleoperator (LeRobot plugin, Conventions #2 and #3).

Works directly with LeRobot's CLI tools:

    lerobot-teleoperate \
        --robot.type=<your_robot> --robot.port=<...> \
        --teleop.type=qt_hmi

get_action() returns the most recent goal-position frame the HMI has sent,
holding the previous value between frames. It also reflects that frame plus the
joint feature list back on the telemetry channel, so the HMI has something live
to display even on the stock CLI path. For real observed joint positions, run
control_service.py instead.
"""
from __future__ import annotations

import time
from typing import Any

from lerobot.teleoperators.teleoperator import Teleoperator

from . import protocol
from .bridge import HmiBridge
from .config_qt_hmi import QtHmiTeleopConfig


class QtHmiTeleop(Teleoperator):
    config_class = QtHmiTeleopConfig
    name = "qt_hmi"

    def __init__(self, config: QtHmiTeleopConfig):
        super().__init__(config)
        self.config = config
        self._bridge: HmiBridge | None = None
        self._last: dict[str, float] = {j: 0.0 for j in config.joints}
        self._seq = 0
        self._last_features = 0.0

    # --- interface contract ------------------------------------------------
    @property
    def action_features(self) -> dict[str, type]:
        return {j: float for j in self.config.joints}

    @property
    def feedback_features(self) -> dict[str, type]:
        return {}

    @property
    def is_connected(self) -> bool:
        return self._bridge is not None

    @property
    def is_calibrated(self) -> bool:
        return True

    def connect(self, calibrate: bool = True) -> None:
        self._bridge = HmiBridge(self.config.action_bind, self.config.telemetry_bind)
        self._bridge.publish_status(True, True, "qt_hmi teleoperator connected")

    def calibrate(self) -> None:
        pass

    def configure(self) -> None:
        pass

    def disconnect(self) -> None:
        if self._bridge is not None:
            self._bridge.publish_status(False, True, "qt_hmi teleoperator disconnected")
            self._bridge.close()
            self._bridge = None

    # --- runtime I/O -------------------------------------------------------
    def get_action(self) -> dict[str, Any]:
        if self._bridge is None:
            raise ConnectionError("QtHmiTeleop is not connected.")

        action, commands = self._bridge.poll_inbound()
        for cmd in commands:
            if cmd in (protocol.CMD_ESTOP, protocol.CMD_HOME):
                self._last = {j: 0.0 for j in self.config.joints}
        if action:
            for k, v in action.items():
                if k in self._last:
                    self._last[k] = float(v)

        # Reflect the commanded frame back to the HMI, and re-advertise features
        # roughly once a second for late-joining subscribers.
        now = time.time()
        self._bridge.publish_observation(dict(self._last), self._seq, True)
        if now - self._last_features > 1.0:
            ranges = {j: (self.config.joint_min, self.config.joint_max) for j in self.config.joints}
            self._bridge.publish_features(list(self.config.joints), ranges)
            self._last_features = now
        self._seq += 1

        return dict(self._last)

    def send_feedback(self, feedback: dict[str, Any]) -> dict[str, Any]:
        # No physical feedback device on a software HMI; nothing to actuate.
        return feedback
