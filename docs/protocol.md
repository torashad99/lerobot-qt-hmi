# Wire protocol

Transport is ZeroMQ. Payloads are single-frame UTF-8 JSON objects. JSON is used
so the C++ client needs only `QJsonDocument`.

## Sockets

The Python side is the server and binds both sockets. The Qt HMI is the client
and connects to both.

| Channel   | Python socket | Qt socket | Default port | Direction     |
|-----------|---------------|-----------|--------------|---------------|
| actions   | PULL (bind)   | PUSH      | 5556         | Qt -> Python  |
| telemetry | PUB (bind)    | SUB       | 5557         | Python -> Qt  |

The action channel carries both action and command frames. The Python side
drains it each cycle and keeps the newest action (latest-wins). The Qt side
sends non-blocking with a small send high-water mark, so back-pressure drops
stale frames rather than stalling the UI.

## Common fields

Every frame has:

- `type` (string): one of the message types below.
- `v` (int): protocol version (currently 1).
- `ts` (float): sender timestamp, seconds since the epoch.
- `seq` (int): monotonic sequence counter from the sender.

## Qt -> Python

### action

    { "type": "action", "seq": 42, "ts": 1721470000.12,
      "action": { "joint_1.pos": 12.5, "joint_2.pos": -30.0 } }

Keys under `action` must match the robot's `action_features` (LeRobot uses the
`joint_N.pos` convention). The HMI sends the full goal vector each frame.

### command

    { "type": "command", "seq": 43, "command": "estop" }

`command` is one of `enable`, `disable`, `home`, `estop`.

## Python -> Qt

### observation

    { "type": "observation", "seq": 1002, "connected": true,
      "state": { "joint_1.pos": 12.4, "joint_2.pos": -29.7 } }

On the control-service path, `state` is the real observed position. On the
plugin path, it reflects the commanded target.

### features

    { "type": "features",
      "joints": ["joint_1.pos", "joint_2.pos"],
      "ranges": { "joint_1.pos": [-100.0, 100.0], "joint_2.pos": [-100.0, 100.0] } }

Sent about once a second so a late-joining HMI can build its sliders. The HMI
uses `ranges` for slider bounds.

### status

    { "type": "status", "connected": true, "calibrated": true,
      "message": "control service up" }
