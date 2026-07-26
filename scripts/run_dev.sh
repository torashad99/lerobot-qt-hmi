#!/usr/bin/env bash
# Bring up the hardware-free demo on a dev machine.
#
#   ./scripts/run_dev.sh service     # start the simulated control service
#   ./scripts/run_dev.sh build-hmi   # configure + build the Qt client
#   ./scripts/run_dev.sh run-hmi     # run the built Qt client
#
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

case "${1:-}" in
  service)
    cd "$ROOT/python"
    python3 -m venv .venv
    # shellcheck disable=SC1091
    source .venv/bin/activate
    pip install -e .            # pyzmq only; add ".[lerobot]" for the plugin path
    echo "Starting simulated control service on tcp://*:5556 (actions) / tcp://*:5557 (telemetry)"
    exec python -m lerobot_teleoperator_qt_hmi.control_service --sim
    ;;
  build-hmi)
    cd "$ROOT/qt-hmi"
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    echo "Built: $ROOT/qt-hmi/build/lerobot_qt_hmi"
    ;;
  run-hmi)
    exec "$ROOT/qt-hmi/build/lerobot_qt_hmi"
    ;;
  *)
    echo "usage: $0 {service|build-hmi|run-hmi}" >&2
    exit 2
    ;;
esac
