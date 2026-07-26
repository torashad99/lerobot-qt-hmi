"""lerobot_teleoperator_qt_hmi

A Qt6 HMI teleoperator for LeRobot, plus a standalone control service and a
ZeroMQ transport that can be used on their own.

The transport modules (protocol, bridge) have no lerobot dependency. The
teleoperator classes require lerobot; if it is not installed they are exposed
as None so the package still imports for transport-only / sim use.
"""
from . import protocol  # noqa: F401
from .bridge import HmiBridge  # noqa: F401

try:
    from .config_qt_hmi import QtHmiTeleopConfig
    from .qt_hmi import QtHmiTeleop
except Exception:  # lerobot not installed
    QtHmiTeleopConfig = None
    QtHmiTeleop = None

__all__ = ["protocol", "HmiBridge", "QtHmiTeleopConfig", "QtHmiTeleop"]
__version__ = "1.0.0"
