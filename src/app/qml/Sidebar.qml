import QtQuick
import QtQuick.Controls

Popup {
    id: root
    property var controller
    signal settingsRequested()

    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(360, parent ? parent.width * 0.82 : 360)
    height: parent ? parent.height : 760
    x: 0
    y: 0
    padding: 0

    background: Rectangle {
        color: "#ffffff"
        radius: 0
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 28
        anchors.rightMargin: 24
        anchors.topMargin: 78
        anchors.bottomMargin: 24
        spacing: 22

        Text {
            text: "Recents"
            font.pixelSize: 24
            font.weight: Font.Bold
            color: "#111111"
        }

        ListView {
            id: recentList
            width: parent.width
            height: parent.height - 142
            clip: true
            spacing: 2
            model: controller.recents
            delegate: Button {
                width: recentList.width
                height: 58
                padding: 0
                onClicked: {
                    controller.loadConversation(modelData.id)
                    root.close()
                }
                background: Rectangle {
                    radius: 14
                    color: down ? "#eeeeee" : "transparent"
                }
                contentItem: Text {
                    text: modelData.title
                    width: parent.width
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    color: "#111111"
                    font.pixelSize: 20
                }
            }
            Text {
                anchors.centerIn: parent
                visible: recentList.count === 0
                text: "No recent chats"
                color: "#999999"
                font.pixelSize: 18
            }
        }

        Row {
            width: parent.width
            spacing: 14
            Button {
                height: 54
                width: parent.width - 100
                text: "Chat"
                font.pixelSize: 18
                font.weight: Font.Bold
                onClicked: {
                    controller.newChat()
                    root.close()
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 27
                    color: "#111111"
                }
            }
            Button {
                width: 82
                height: 54
                text: "⚙"
                font.pixelSize: 24
                onClicked: {
                    root.close()
                    root.settingsRequested()
                }
                background: Rectangle {
                    radius: 27
                    color: "#f2f2f2"
                    border.color: "#ffffff"
                }
            }
        }
    }
}
