"""A hardware-free stand-in for a LeRobot Robot.

It implements just the slice of the Robot interface the control service uses
(connect / disconnect / is_connected / get_observation / send_action /
action_features), so you can drive the entire HMI stack end to end without a
robot and without installing lerobot. Swap it for your real Robot subclass in
control_service.build_robot() when you have hardware.
"""
from __future__ import annotations

import math
import time


class SimRobot:
    def __init__(self, n_joints: int = 5, joint_range: tuple[float, float] = (-100.0, 100.0)):
        self._names = [f"joint_{i}.pos" for i in range(1, n_joints + 1)]
        self._min, self._max = joint_range
        self._state = {n: 0.0 for n in self._names}
        self._target = dict(self._state)
        self._connected = False
        self._t0 = time.time()

    @property
    def joint_names(self) -> list[str]:
        return list(self._names)

    @property
    def ranges(self) -> dict[str, tuple[float, float]]:
        return {n: (self._min, self._max) for n in self._names}

    @property
    def action_features(self) -> dict[str, type]:
        return {n: float for n in self._names}

    @property
    def is_connected(self) -> bool:
        return self._connected

    def connect(self, calibrate: bool = True) -> None:
        self._connected = True

    def disconnect(self) -> None:
        self._connected = False

    def get_observation(self) -> dict[str, float]:
        # Ease current state toward the commanded target, with a small idle sway
        # on untouched joints so the UI visibly reacts.
        t = time.time() - self._t0
        for i, n in enumerate(self._names):
            cur = self._state[n]
            cur += (self._target[n] - cur) * 0.2
            if self._target[n] == 0.0:
                cur += math.sin(t + i) * 0.5
            self._state[n] = max(self._min, min(self._max, cur))
        return dict(self._state)

    def send_action(self, action: dict[str, float]) -> dict[str, float]:
        for n, v in action.items():
            if n in self._target:
                self._target[n] = max(self._min, min(self._max, float(v)))
        return action
