import QtQuick
import QtQuick.Controls

Popup {
    id: root
    property var controller
    property string page: "effort"

    modal: true
    dim: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onOpened: page = "effort"
    width: Math.min(300, parent ? parent.width - 72 : 300)
    height: page === "models" ? 548 : 512
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
            onClicked: root.page = root.page === "providers" ? "effort" : "providers"
            background: Rectangle { color: "transparent" }
            contentItem: Item {
                Text {
                    anchors.left: parent.left
                    anchors.right: chevronWrap.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: controller.selectedProvider + " / " + controller.selectedMenuTitle
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: "#111111"
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                Item {
                    id: chevronWrap
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 36
                    height: parent.height

                    Chevron {
                        anchors.centerIn: parent
                        direction: root.page === "providers" ? "down" : "right"
                        color: "#111111"
                        strokeWidth: 2.5
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#dddddd"
        }

        Column {
            visible: root.page === "effort"
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
                    contentItem: Item {
                        Text {
                            id: effortCheck
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 36
                            text: controller.selectedEffort === modelData ? "✓" : ""
                            font.pixelSize: 20
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                        }
                        Text {
                            anchors.left: effortCheck.right
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            font.pixelSize: 22
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Column {
            visible: root.page === "providers"
            width: parent.width
            spacing: 5
            Text {
                text: "Provider"
                color: "#999999"
                font.pixelSize: 18
                leftPadding: 35
                height: 38
                verticalAlignment: Text.AlignVCenter
            }
            Repeater {
                model: controller.providerOptions
                delegate: Button {
                    width: parent.width
                    height: 56
                    padding: 0
                    onClicked: {
                        controller.selectedProvider = modelData.name
                        root.page = "models"
                    }
                    background: Rectangle {
                        radius: 14
                        color: down ? "#e6e6e6" : "transparent"
                    }
                    contentItem: Item {
                        Text {
                            id: providerCheck
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 36
                            text: controller.selectedProvider === modelData.name ? "✓" : ""
                            font.pixelSize: 20
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                        }
                        Text {
                            anchors.left: providerCheck.right
                            anchors.right: providerChevron.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.name
                            font.pixelSize: 22
                            color: "#111111"
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        Item {
                            id: providerChevron
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 28
                            height: parent.height
                            Chevron {
                                anchors.centerIn: parent
                                direction: "right"
                                color: "#bbbbbb"
                                strokeWidth: 2.2
                            }
                        }
                    }
                }
            }
        }

        Flickable {
            visible: root.page === "models"
            width: parent.width
            height: root.height - 26 * 2 - 56 - 14 - 1 - 14
            contentWidth: width
            contentHeight: modelColumn.height
            clip: true

            Column {
                id: modelColumn
                width: parent.width
                spacing: 5
                Text {
                    text: controller.selectedProvider
                    color: "#999999"
                    font.pixelSize: 18
                    leftPadding: 35
                    height: 38
                    verticalAlignment: Text.AlignVCenter
                }
                Repeater {
                    model: controller.modelOptions
                    delegate: Button {
                        width: parent.width
                        height: 52
                        padding: 0
                        onClicked: {
                            controller.selectedModel = modelData.displayName
                            root.close()
                        }
                        background: Rectangle {
                            radius: 14
                            color: down ? "#e6e6e6" : "transparent"
                        }
                        contentItem: Item {
                            Text {
                                id: modelCheck
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 36
                                text: controller.selectedModel === modelData.displayName ? "✓" : ""
                                font.pixelSize: 20
                                color: "#111111"
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignLeft
                            }
                            Text {
                                anchors.left: modelCheck.right
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.displayName
                                font.pixelSize: 20
                                color: "#111111"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }
}
