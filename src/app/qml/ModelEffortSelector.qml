import QtQuick
import QtQuick.Controls

Popup {
    id: root
    property var controller
    property bool modelMode: false

    modal: true
    dim: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(300, parent ? parent.width - 72 : 300)
    height: modelMode ? 454 : 392
    x: parent ? (parent.width - width) / 2 : 80
    y: 78

    background: Rectangle {
        color: "#f4f4f4"
        radius: 30
        border.color: "#ffffff"
        border.width: 1
    }

    Column {
        anchors.fill: parent
        anchors.margins: 26
        spacing: 14

        Button {
            width: parent.width
            height: 56
            padding: 0
            onClicked: root.modelMode = !root.modelMode
            background: Rectangle { color: "transparent" }
            contentItem: Row {
                Text {
                    width: parent.width - 36
                    text: controller.selectedMenuTitle
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: "#111111"
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    text: root.modelMode ? "v" : ">"
                    font.pixelSize: 28
                    color: "#111111"
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#dddddd"
        }

        Column {
            visible: !root.modelMode
            width: parent.width
            spacing: 5
            Text {
                text: "Intelligence"
                color: "#999999"
                font.pixelSize: 18
                leftPadding: 35
                height: 38
                verticalAlignment: Text.AlignVCenter
            }
            Repeater {
                model: controller.effortOptions
                delegate: Button {
                    width: parent.width
                    height: 56
                    padding: 0
                    onClicked: {
                        controller.selectedEffort = modelData
                        root.close()
                    }
                    background: Rectangle {
                        radius: 14
                        color: down ? "#e6e6e6" : "transparent"
                    }
                    contentItem: Row {
                        Text {
                            width: 36
                            text: controller.selectedEffort === modelData ? "✓" : ""
                            font.pixelSize: 20
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: modelData
                            font.pixelSize: 22
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }

        Column {
            visible: root.modelMode
            width: parent.width
            spacing: 5
            Repeater {
                model: controller.modelOptions
                delegate: Button {
                    width: parent.width
                    height: 60
                    padding: 0
                    onClicked: {
                        controller.selectedModel = modelData.displayName
                        root.close()
                    }
                    background: Rectangle {
                        radius: 14
                        color: down ? "#e6e6e6" : "transparent"
                    }
                    contentItem: Row {
                        Text {
                            width: 36
                            text: controller.selectedModel === modelData.displayName ? "✓" : ""
                            font.pixelSize: 20
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: modelData.displayName
                            font.pixelSize: 22
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }
}
