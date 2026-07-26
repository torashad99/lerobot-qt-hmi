# lerobot-qt-hmi


Control a LeRobot robot from a Qt6 C++/QML HMI, with a Python side that either
plugs into LeRobot's own tooling or runs as a standalone control service.

The design keeps the language boundary clean: Qt6 stays in C++/QML (which is
what `meta-qt6` builds well on Yocto), LeRobot stays in Python, and the two
talk over a small ZeroMQ + JSON bridge. The HMI is a supervisor, not part of
the hard control loop.

## Architecture

<img width="2752" height="1536" alt="lerobot-qt-hmi-image" src="https://github.com/user-attachments/assets/6df54215-bde9-462f-ab15-cf7d71267983" />

There are two ways to drive the robot, sharing the same wire protocol and the
same HMI:

1. Plugin path. `qt_hmi` is a LeRobot `Teleoperator`, auto-discovered by
   `lerobot-teleoperate` and `lerobot-record`. LeRobot builds the robot and
   runs the loop; the HMI's sliders become the teleoperation source, so you get
   dataset recording for free. The HMI reflects the commanded targets.

2. Control-service path. `control_service.py` owns its own loop and streams the
   real observed joint positions back to the HMI. Recommended when the live
   view needs to show the robot actually following. Includes a `--sim` mode
   that needs no hardware and no lerobot install, so you can exercise the whole
   stack immediately.

## Layout

    python/                     LeRobot teleoperator plugin + control service
      lerobot_teleoperator_qt_hmi/
        protocol.py             wire format (JSON), no lerobot dependency
        bridge.py               server-side ZeroMQ transport
        sim_robot.py            hardware-free Robot stand-in
        control_service.py      standalone loop, --sim demo, CLI entry point
        config_qt_hmi.py        TeleoperatorConfig (registered as "qt_hmi")
        qt_hmi.py               Teleoperator implementation
      pyproject.toml
    qt-hmi/                      Qt6 C++/QML client
      src/
        RobotBridge.*           QML-facing controller, owns the sockets/thread
        TelemetryWorker.*       SUB socket loop, runs in a worker QThread
        JointModel.*            QAbstractListModel backing the joint table
        Protocol.h              wire-format constants
        main.cpp
      qml/Main.qml, qml/JointRow.qml
      CMakeLists.txt
    yocto/                       example recipes (Qt app + Python plugin)
    systemd/                     units for the service and the HMI
    docs/protocol.md             wire protocol reference
    scripts/run_dev.sh           dev convenience script

## Quickstart (no hardware)

Two terminals. First, the simulated control service:

    cd python
    python3 -m venv .venv && source .venv/bin/activate
    pip install -e .                    # pyzmq only
    python -m lerobot_teleoperator_qt_hmi.control_service --sim

Then build and run the HMI (needs Qt6, libzmq, cppzmq):

    cd qt-hmi
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ./build/lerobot_qt_hmi

Click Start. The sliders send goal positions; the "obs" readout shows the
simulated robot easing toward each target. `scripts/run_dev.sh` wraps these
steps.

Dev dependencies:

- Debian/Ubuntu: `apt install qt6-declarative-dev libzmq3-dev cppzmq-dev cmake ninja-build`
- Python: `pip install pyzmq` (the editable install pulls it in)

## Plugin path (full LeRobot integration)

Install with the extra and wrap your own robot as a LeRobot `Robot` subclass
(see the LeRobot "Bring Your Own Hardware" guide):

    cd python
    pip install -e ".[lerobot]"

    lerobot-teleoperate \
        --robot.type=<your_robot> --robot.port=<...> \
        --teleop.type=qt_hmi

The `qt_hmi` teleoperator binds the ZeroMQ sockets; point the HMI at that host
and Start. Swap `lerobot-teleoperate` for `lerobot-record` to collect a
dataset with the HMI as the teleoperation source.

Note: the teleoperator's `joints` (in `config_qt_hmi.py`) must match your
robot's `action_features` keys so LeRobot can map actions through.

## Real hardware via the control service

Edit `build_robot()` in `control_service.py` to construct your `Robot`
subclass, then drop `--sim`:

    from my_robot_pkg import MyCoolRobot, MyCoolRobotConfig
    return MyCoolRobot(MyCoolRobotConfig(port=args.port))

## Yocto notes

- Qt6: `meta-qt6` provides `qtbase`, `qtdeclarative`, and the `cmake_qt6`
  class. The `qt-hmi` recipe uses it directly.
- ZeroMQ: `zeromq` and `cppzmq` from `meta-openembedded/meta-oe`;
  `python3-pyzmq` from `meta-python`.
- Keep PySide6 out of the image. Cross-compiling shiboken/PySide6 is the
  painful path; a C++/QML HMI plus a Python service avoids it entirely.
- PyTorch footprint: teleoperation and the sim need no torch. On-device policy
  inference is a separate weight class and is not packaged here; use a venv or
  a dedicated lerobot recipe if you need it on target.
- Run both halves under systemd (`systemd/`), so the service and HMI restart
  independently.

## Scope

Control and telemetry are fully implemented end to end. Camera display is left
as a documented extension: the idiomatic embedded route is a GStreamer pipeline
into a Qt6 `QVideoSink` (hardware-decoded on most SoCs), on its own channel and
never through the command socket. LeRobot still reads its own camera for any
policy; the HMI display is a parallel path. The wire protocol has room for a
separate video endpoint alongside the two control sockets.
