"""Standalone control service.

This is the recommended path when you want the HMI to show real observed joint
positions (not just the values it is commanding). It owns the control loop:

    read latest action from HMI  ->  robot.send_action()
    robot.get_observation()      ->  publish to HMI

Run the hardware-free demo:

    python -m lerobot_teleoperator_qt_hmi.control_service --sim

For real hardware, edit build_robot() to construct your LeRobot Robot subclass.
The plugin path (lerobot-teleoperate --teleop.type=qt_hmi) is the alternative:
it lets LeRobot build the robot and handles dataset recording, but only reflects
commanded targets back to the HMI. Both paths speak the same wire protocol.
"""
from __future__ import annotations

import argparse
import signal
import time

from . import protocol
from .bridge import HmiBridge


def build_robot(args):
    if args.sim:
        from .sim_robot import SimRobot

        return SimRobot(n_joints=args.joints)

    # --- Real hardware -----------------------------------------------------
    # Construct your LeRobot Robot subclass here, e.g.:
    #
    #   from my_robot_pkg import MyCoolRobot, MyCoolRobotConfig
    #   return MyCoolRobot(MyCoolRobotConfig(port=args.port))
    #
    raise SystemExit(
        "No robot configured. Run with --sim, or edit build_robot() in "
        "control_service.py to construct your LeRobot Robot subclass."
    )


def handle_command(robot, cmd: str, bridge: HmiBridge) -> None:
    if cmd == protocol.CMD_ESTOP:
        # Minimal safe response: command a hold at zero. Real hardware should
        # also disable torque on the motor bus here.
        try:
            robot.send_action({n: 0.0 for n in robot.action_features})
        except Exception:
            pass
        bridge.publish_status(robot.is_connected, True, "E-STOP")
    elif cmd == protocol.CMD_HOME:
        try:
            robot.send_action({n: 0.0 for n in robot.action_features})
        except Exception:
            pass
        bridge.publish_status(robot.is_connected, True, "homing")
    # CMD_ENABLE / CMD_DISABLE are left as hooks for real hardware.


def run(args) -> None:
    robot = build_robot(args)
    bridge = HmiBridge(action_bind=args.action_bind, telemetry_bind=args.telemetry_bind)

    running = {"v": True}

    def _stop(*_):
        running["v"] = False

    signal.signal(signal.SIGINT, _stop)
    signal.signal(signal.SIGTERM, _stop)

    robot.connect()

    names = getattr(robot, "joint_names", None) or list(robot.action_features.keys())
    ranges = getattr(robot, "ranges", {n: (-100.0, 100.0) for n in names})
    bridge.publish_status(True, True, "control service up")

    period = 1.0 / args.rate
    seq = 0
    last_features = 0.0
    try:
        while running["v"]:
            loop_start = time.time()

            action, commands = bridge.poll_inbound()
            for cmd in commands:
                handle_command(robot, cmd, bridge)
            if action:
                robot.send_action(action)

            obs = robot.get_observation()
            state = {k: float(v) for k, v in obs.items() if str(k).endswith(".pos")}
            bridge.publish_observation(state, seq, robot.is_connected)

            # Re-advertise features about once a second so a late-joining HMI can
            # build its UI (PUB/SUB drops messages sent before a subscriber joins).
            if loop_start - last_features > 1.0:
                bridge.publish_features(names, ranges)
                last_features = loop_start

            seq += 1

            dt = time.time() - loop_start
            if dt < period:
                time.sleep(period - dt)
    finally:
        bridge.publish_status(False, True, "control service stopping")
        try:
            robot.disconnect()
        except Exception:
            pass
        bridge.close()


def main() -> None:
    p = argparse.ArgumentParser(description="Qt HMI <-> LeRobot control service")
    p.add_argument("--sim", action="store_true", help="run a hardware-free simulated robot")
    p.add_argument("--joints", type=int, default=5, help="joint count for --sim")
    p.add_argument("--rate", type=float, default=50.0, help="control loop rate (Hz)")
    p.add_argument("--action-bind", default="tcp://*:5556")
    p.add_argument("--telemetry-bind", default="tcp://*:5557")
    p.add_argument("--port", default=None, help="serial port for real hardware")
    run(p.parse_args())


if __name__ == "__main__":
    main()
