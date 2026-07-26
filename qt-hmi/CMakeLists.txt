cmake_minimum_required(VERSION 3.21)
project(lerobot_qt_hmi VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(PkgConfig REQUIRED)
# libzmq via pkg-config. cppzmq is the header-only zmq.hpp wrapper and is
# expected on the include path (Debian: libzmq3-dev + cppzmq-dev;
# Yocto: zeromq + cppzmq from meta-oe).
pkg_check_modules(ZMQ REQUIRED IMPORTED_TARGET libzmq)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(lerobot_qt_hmi
    src/main.cpp
    src/RobotBridge.cpp
    src/RobotBridge.h
    src/TelemetryWorker.cpp
    src/TelemetryWorker.h
    src/JointModel.cpp
    src/JointModel.h
    src/Protocol.h
)

qt_add_qml_module(lerobot_qt_hmi
    URI LeRobotHmi
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/JointRow.qml
)

target_link_libraries(lerobot_qt_hmi
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        PkgConfig::ZMQ
)

install(TARGETS lerobot_qt_hmi
    RUNTIME DESTINATION bin
)
