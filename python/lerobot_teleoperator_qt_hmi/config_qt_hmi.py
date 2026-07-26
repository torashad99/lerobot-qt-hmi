"""Configuration for the Qt HMI teleoperator (LeRobot plugin, Convention #2)."""
from __future__ import annotations

from dataclasses import dataclass

from lerobot.teleoperators.config import TeleoperatorConfig


@TeleoperatorConfig.register_subclass("qt_hmi")
@dataclass
class QtHmiTeleopConfig(TeleoperatorConfig):
    # ZeroMQ endpoints. This teleoperator is the server and binds them; the Qt
    # HMI is the client and connects to them.
    action_bind: str = "tcp://*:5556"
    telemetry_bind: str = "tcp://*:5557"

    # Joint names in the robot's action space. These keys MUST match your
    # robot's action_features so lerobot-teleoperate can map them through.
    joints: tuple[str, ...] = (
        "joint_1.pos",
        "joint_2.pos",
        "joint_3.pos",
        "joint_4.pos",
        "joint_5.pos",
    )

    # Slider bounds advertised to the HMI when no calibration ranges are known.
    joint_min: float = -100.0
    joint_max: float = 100.0
