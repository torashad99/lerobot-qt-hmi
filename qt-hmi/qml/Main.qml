import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: win
    width: 640
    height: 720
    visible: true
    title: qsTr("LeRobot Qt HMI")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // --- Connection header ---
        Frame {
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Rectangle {
                        width: 14; height: 14; radius: 7
                        color: robot.connected ? "#1D9E75" : "#A32D2D"
                    }
                    Label {
                        text: robot.connected ? qsTr("Connected") : qsTr("Disconnected")
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Label { text: qsTr("Host:") }
                    TextField {
                        id: hostField
                        text: robot.host
                        implicitWidth: 140
                        onEditingFinished: robot.host = text
                    }
                    Button {
                        text: qsTr("Start")
                        onClicked: robot.start()
                    }
                    Button {
                        text: qsTr("Stop")
                        onClicked: robot.stop()
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: robot.statusMessage
                    color: "#5F5E5A"
                    elide: Text.ElideRight
                }
            }
        }

        // --- Joint table ---
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 6
                Label {
                    text: qsTr("Joints")
                    font.bold: true
                }
                ListView {
                    id: jointList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: robot.jointModel
                    delegate: JointRow {
                        width: jointList.width
                    }
                }
            }
        }

        // --- Control buttons ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Button {
                text: qsTr("Home")
                onClicked: robot.sendCommand("home")
            }
            Button {
                text: qsTr("Enable")
                onClicked: robot.sendCommand("enable")
            }
            Button {
                text: qsTr("Disable")
                onClicked: robot.sendCommand("disable")
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("E-STOP")
                palette.button: "#A32D2D"
                palette.buttonText: "white"
                onClicked: robot.sendCommand("estop")
            }
        }
    }
}
