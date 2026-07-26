SUMMARY = "LeRobot Qt6 HMI client"
DESCRIPTION = "Qt6 C++/QML HMI that controls a LeRobot robot over a ZeroMQ bridge."
HOMEPAGE = "https://example.invalid/lerobot-qt-hmi"

# Switch to a real SPDX identifier and LIC_FILES_CHKSUM once you add a LICENSE
# file to the source tree. CLOSED keeps the recipe building in the meantime.
LICENSE = "CLOSED"

# meta-qt6 provides qtbase / qtdeclarative and the cmake_qt6 class.
# zeromq and cppzmq come from meta-oe.
DEPENDS = "qtbase qtdeclarative zeromq cppzmq"
RDEPENDS:${PN} = "qtbase qtdeclarative"

# Point this at however you deliver the source (git, tarball, or a local copy
# beside this recipe). The layout below assumes the qt-hmi/ directory.
SRC_URI = "file://qt-hmi"
S = "${WORKDIR}/qt-hmi"

inherit cmake_qt6

FILES:${PN} += "${bindir}/lerobot_qt_hmi"

# On a Wayland/Weston target you usually want:
#   RDEPENDS:${PN} += "qtwayland"
# and to launch with QT_QPA_PLATFORM=wayland (see systemd/lerobot-hmi.service).
