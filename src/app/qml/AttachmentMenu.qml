import QtQuick
import QtQuick.Controls

Popup {
    id: root
    property var controller
    property var store
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
            iconName: "camera"
            onClicked: {
                root.close()
                root.chooserRequested("camera")
            }
        }
        AttachmentMenuRow {
            label: "Photos"
            iconName: "photos"
            onClicked: {
                root.close()
                root.chooserRequested("photos")
            }
        }
        AttachmentMenuRow {
            label: "Files"
            iconName: "files"
            onClicked: {
                root.close()
                root.chooserRequested("files")
            }
        }
        Text {
            width: parent.width
            text: "Plugins"
            color: "#777777"
            font.pixelSize: 15
            font.weight: Font.Bold
            leftPadding: 8
        }
        Button {
            id: jsEngineRow
            height: 66
            width: parent.width
            padding: 0
            background: Rectangle {
                color: jsEngineRow.down ? "#e5e5e5" : jsEngineRow.hovered ? "#eeeeee" : "transparent"
                radius: 18
            }
            contentItem: Row {
                spacing: 20
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    width: 54
                    height: 54
                    radius: 27
                    color: "#e7e7e7"
                    AttachmentMenuIcon {
                        anchors.centerIn: parent
                        iconName: "webBrowser"
                        stroke: "#111111"
                        accent: "#111111"
                    }
                }
                Text {
                    width: parent.width - 142
                    text: "Code Interpreter"
                    color: "#111111"
                    font.pixelSize: 23
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                }
                Switch {
                    id: jsEngineSwitch
                    anchors.verticalCenter: parent.verticalCenter
                    checked: root.store ? root.store.javaScriptUseEnabled : false
                    onToggled: {
                        if (root.store)
                            root.store.javaScriptUseEnabled = checked
                    }
                }
            }
        }
        Button {
            id: webBrowserRow
            height: 66
            width: parent.width
            padding: 0
            background: Rectangle {
                color: webBrowserRow.down ? "#e5e5e5" : webBrowserRow.hovered ? "#eeeeee" : "transparent"
                radius: 18
            }
            contentItem: Row {
                spacing: 20
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    width: 54
                    height: 54
                    radius: 27
                    color: "#e7e7e7"
                    AttachmentMenuIcon {
                        anchors.centerIn: parent
                        iconName: "plugins"
                        stroke: "#111111"
                        accent: "#111111"
                    }
                }
                Text {
                    width: parent.width - 142
                    text: "Web Browser"
                    color: "#111111"
                    font.pixelSize: 23
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                }
                Switch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: root.store ? root.store.webBrowserEnabled : false
                    onToggled: {
                        if (root.store)
                            root.store.webBrowserEnabled = checked
                    }
                }
            }
        }
    }
}
