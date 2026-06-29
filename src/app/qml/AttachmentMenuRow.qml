import QtQuick
import QtQuick.Controls

Button {
    id: row
    property string label
    property string iconText

    height: 66
    width: parent ? parent.width : 280
    padding: 0
    background: Rectangle {
        color: row.down ? "#e7e7e7" : "transparent"
        radius: 18
    }
    contentItem: Row {
        spacing: 20
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            width: 54
            height: 54
            radius: 27
            color: "#e9e9e9"
            Text {
                anchors.centerIn: parent
                text: row.iconText
                font.pixelSize: 24
                color: "#111111"
            }
        }
        Text {
            text: row.label
            color: "#111111"
            font.pixelSize: 25
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
