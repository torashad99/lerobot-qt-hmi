import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Delegate for one joint. The slider owns the commanded target; the observed
// position from telemetry is shown separately so you can see the robot follow.
RowLayout {
    id: row
    spacing: 12

    Label {
        text: model.name
        Layout.preferredWidth: 90
        elide: Text.ElideRight
    }

    Slider {
        id: slider
        Layout.fillWidth: true
        from: model.min
        to: model.max
        value: model.target
        onMoved: robot.setJointTarget(model.name, value)
    }

    // Commanded target.
    Label {
        text: qsTr("tgt %1").arg(slider.value.toFixed(1))
        Layout.preferredWidth: 74
        color: "#534AB7"
        horizontalAlignment: Text.AlignRight
    }

    // Observed position.
    Label {
        text: qsTr("obs %1").arg(Number(model.value).toFixed(1))
        Layout.preferredWidth: 74
        color: "#0F6E56"
        horizontalAlignment: Text.AlignRight
    }
}
