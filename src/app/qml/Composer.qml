import QtQuick
import QtQuick.Controls

Item {
    id: root
    property var controller
    property int editRow: -1
    signal addClicked()
    signal modelClicked()

    function startEdit(row, messageText) {
        editRow = row
        textArea.text = messageText
        textArea.forceActiveFocus()
        textArea.cursorPosition = textArea.text.length
    }

    function finishEdit() {
        editRow = -1
        textArea.text = ""
    }

    implicitHeight: column.implicitHeight

    Column {
        id: column
        width: parent.width
        spacing: 8

        Rectangle {
            visible: root.editRow >= 0
            width: parent.width
            implicitHeight: editBannerRow.implicitHeight + 14
            radius: 8
            color: "#eef1f5"
            border.color: "#dce2ea"

            Row {
                id: editBannerRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 7
                spacing: 8

                Text {
                    width: Math.max(0, parent.width - editModelButton.width - cancelEditButton.width - 16)
                    text: "Editing"
                    color: "#3d4756"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                Button {
                    id: editModelButton
                    width: Math.min(190, Math.max(112, editModelLabel.implicitWidth + 24))
                    height: 30
                    padding: 0
                    onClicked: root.modelClicked()
                    contentItem: Text {
                        id: editModelLabel
                        text: controller.selectorLabel
                        color: "#26313f"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle { radius: 8; color: "#ffffff"; border.color: "#dce2ea" }
                }

                Button {
                    id: cancelEditButton
                    width: 70
                    height: 30
                    text: "Cancel"
                    padding: 0
                    onClicked: root.finishEdit()
                    contentItem: Text {
                        text: parent.text
                        color: "#526070"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 8; color: "#ffffff"; border.color: "#dce2ea" }
                }
            }
        }

        Flow {
            id: attachmentFlow
            width: parent.width
            visible: root.editRow < 0 && controller.pendingAttachments.length > 0
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
                    visible: root.editRow < 0
                    text: "+"
                    font.pixelSize: 33
                    padding: 0
                    onClicked: root.addClicked()
                    background: Rectangle { radius: 21; color: "transparent" }
                }

                TextArea {
                    id: textArea
                    width: Math.max(0, parent.width - (addButton.visible ? addButton.width + 8 : 0) - micButton.width - sendButton.width - 24)
                    anchors.verticalCenter: parent.verticalCenter
                    placeholderText: root.editRow >= 0 ? "Edit message" : "Ask AI"
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
                    visible: root.editRow < 0
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
                    text: controller.busy ? "…" : (root.editRow >= 0 ? "✓" : "↑")
                    font.pixelSize: 25
                    font.weight: Font.Bold
                    onClicked: {
                        if (root.editRow >= 0) {
                            controller.amendUserMessage(root.editRow, textArea.text)
                            root.finishEdit()
                        } else {
                            controller.sendMessage(textArea.text)
                            textArea.text = ""
                        }
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
