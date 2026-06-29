import QtQuick
import QtQuick.Controls

Button {
    id: root
    property int buttonSize: 52
    property color fill: "#f7f7f7"
    property color stroke: "#ffffff"
    property color foreground: "#151515"

    width: buttonSize
    height: buttonSize
    padding: 0
    font.pixelSize: 24
    font.weight: Font.Normal
    contentItem: Text {
        text: root.text
        color: root.foreground
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: root.buttonSize / 2
        color: root.down ? "#eeeeee" : root.fill
        border.color: root.stroke
        border.width: 1
    }
}
