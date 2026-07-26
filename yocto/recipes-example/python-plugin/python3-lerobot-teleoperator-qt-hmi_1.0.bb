SUMMARY = "Qt HMI teleoperator plugin for LeRobot (ZeroMQ bridge + control service)"
LICENSE = "CLOSED"

SRC_URI = "file://python"
S = "${WORKDIR}/python"

inherit setuptools3

# The transport and the --sim control service need only pyzmq. python3-pyzmq
# is in meta-python (meta-openembedded).
RDEPENDS:${PN} = "python3-pyzmq python3-json"

# lerobot itself (and its torch/numpy stack) is heavy and is NOT packaged here.
# If you need the plugin path (lerobot-teleoperate --teleop.type=qt_hmi) on the
# target, provide a lerobot recipe or install it into a venv. For teleoperation
# and the sim, the control service alone is enough:
#   lerobot-qt-control --sim
