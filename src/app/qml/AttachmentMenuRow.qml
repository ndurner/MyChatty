import QtQuick
import QtQuick.Controls

Button {
    id: row
    property string label
    property string iconName

    height: 66
    width: parent ? parent.width : 280
    padding: 0
    hoverEnabled: true
    background: Rectangle {
        color: row.down ? "#e5e5e5" : row.hovered ? "#eeeeee" : "transparent"
        radius: 18
    }
    contentItem: Row {
        spacing: 20
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            width: 54
            height: 54
            radius: 27
            color: row.down ? "#d9d9d9" : row.hovered ? "#e4e4e4" : "#e7e7e7"
            AttachmentMenuIcon {
                anchors.centerIn: parent
                iconName: row.iconName
                stroke: "#111111"
                accent: "#111111"
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
