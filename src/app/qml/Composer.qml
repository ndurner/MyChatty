import QtQuick
import QtQuick.Controls

Item {
    id: root
    property var controller
    signal addClicked()

    implicitHeight: column.implicitHeight

    Column {
        id: column
        width: parent.width
        spacing: 8

        Flow {
            id: attachmentFlow
            width: parent.width
            visible: controller.pendingAttachments.length > 0
            spacing: 8
            leftPadding: 10
            rightPadding: 10
            Repeater {
                model: controller.pendingAttachments
                delegate: Rectangle {
                    height: 34
                    width: Math.min(label.implicitWidth + 42, attachmentFlow.width - 20)
                    radius: 17
                    color: "#eeeeee"
                    border.color: "#dddddd"
                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 6
                        spacing: 6
                        Text {
                            id: label
                            text: modelData.fileName
                            width: parent.width - 30
                            elide: Text.ElideMiddle
                            font.pixelSize: 13
                            color: "#333333"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Button {
                            width: 22
                            height: 22
                            anchors.verticalCenter: parent.verticalCenter
                            text: "x"
                            padding: 0
                            font.pixelSize: 12
                            onClicked: controller.removePendingAttachment(modelData.id)
                            background: Rectangle { radius: 11; color: "#dddddd" }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: composerBox
            width: parent.width
            height: Math.max(58, Math.min(118, textArea.implicitHeight + 22))
            radius: height / 2
            color: "#fbfbfb"
            border.color: "#ffffff"
            border.width: 1

            Row {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 8

                Button {
                    id: addButton
                    width: 42
                    height: 42
                    anchors.verticalCenter: parent.verticalCenter
                    text: "+"
                    font.pixelSize: 33
                    padding: 0
                    onClicked: root.addClicked()
                    background: Rectangle { radius: 21; color: "transparent" }
                }

                TextArea {
                    id: textArea
                    width: Math.max(0, parent.width - addButton.width - micButton.width - sendButton.width - 32)
                    anchors.verticalCenter: parent.verticalCenter
                    placeholderText: "Ask AI"
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.Wrap
                    font.pixelSize: 22
                    color: "#111111"
                    placeholderTextColor: "#999999"
                    background: null
                    topPadding: 8
                    bottomPadding: 8
                    Keys.onReturnPressed: function(event) {
                        if ((event.modifiers & Qt.ShiftModifier) === 0) {
                            event.accepted = true
                            sendButton.clicked()
                        }
                    }
                }

                Button {
                    id: micButton
                    width: 40
                    height: 40
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: controller.notifyNotImplemented("Microphone")
                    contentItem: Item {
                        MicrophoneIcon {
                            anchors.centerIn: parent
                            color: "#111111"
                        }
                    }
                    background: Rectangle { radius: 20; color: "transparent" }
                }

                Button {
                    id: sendButton
                    width: 48
                    height: 48
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: !controller.busy
                    text: controller.busy ? "…" : "↑"
                    font.pixelSize: 25
                    font.weight: Font.Bold
                    onClicked: {
                        controller.sendMessage(textArea.text)
                        textArea.text = ""
                    }
                    contentItem: Text {
                        text: sendButton.text
                        color: "#ffffff"
                        font: sendButton.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 24
                        color: sendButton.enabled ? "#000000" : "#777777"
                    }
                }
            }
        }
    }
}
