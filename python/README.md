# lerobot_teleoperator_qt_hmi

A Qt6 HMI teleoperator for LeRobot, plus a standalone control service.

- `qt_hmi.py` / `config_qt_hmi.py`: the LeRobot `Teleoperator` plugin, discovered
  automatically by `lerobot-teleoperate` and `lerobot-record` as `--teleop.type=qt_hmi`.
- `control_service.py`: a standalone fixed-rate loop that streams real observations
  to the HMI. Includes a `--sim` mode that needs no hardware and no lerobot.
- `bridge.py` / `protocol.py`: the ZeroMQ + JSON transport, usable on their own.

Install for the sim/transport only:

    pip install -e .

Install with the LeRobot plugin path:

    pip install -e ".[lerobot]"

See the repository root README for the full architecture and the Qt client.
