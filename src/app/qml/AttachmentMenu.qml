import QtQuick
import QtQuick.Controls

Popup {
    id: root
    property var controller
    signal chooserRequested(string origin)

    modal: true
    dim: false
    focus: true
    width: Math.min(330, parent ? parent.width - 28 : 330)
    height: menuColumn.implicitHeight + 34
    x: 14
    y: parent ? parent.height - height - 150 : 360
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        color: "#f4f4f4"
        radius: 28
        border.color: "#ffffff"
        border.width: 1
    }

    Column {
        id: menuColumn
        anchors.fill: parent
        anchors.margins: 17
        spacing: 12

        AttachmentMenuRow {
            label: "Camera"
            iconText: "▣"
            onClicked: {
                root.close()
                root.chooserRequested("camera")
            }
        }
        AttachmentMenuRow {
            label: "Photos"
            iconText: "▧"
            onClicked: {
                root.close()
                root.chooserRequested("photos")
            }
        }
        AttachmentMenuRow {
            label: "Files"
            iconText: "⌇"
            onClicked: {
                root.close()
                root.chooserRequested("files")
            }
        }
        AttachmentMenuRow {
            label: "Plugins"
            iconText: "⌘"
            onClicked: {
                root.close()
                controller.notifyNotImplemented("Plugins")
            }
        }
    }
}
