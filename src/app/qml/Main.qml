import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    visible: true
    width: 430
    height: 920
    minimumWidth: 360
    minimumHeight: 640
    title: "MyChatty"
    color: "#ffffff"

    property string chooserOrigin: "files"
    property int e2eStep: 0

    Rectangle {
        anchors.fill: parent
        color: "#ffffff"
    }

    Row {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.topMargin: 48
        height: 58
        spacing: 14
        z: 2
        property real selectorButtonWidth: chatList.count > 0 ? Math.min(300, header.width - 188)
                                                              : Math.min(270, header.width - 170)

        IconButton {
            text: "☰"
            buttonSize: 52
            font.pixelSize: 25
            onClicked: sidebar.open()
        }

        Button {
            height: 52
            width: header.selectorButtonWidth
            text: chatController.selectorLabel
            font.pixelSize: chatController.selectorLabel.length > 18 ? 18 : 22
            onClicked: selector.open()
            contentItem: Text {
                text: parent.text
                color: "#111111"
                font: parent.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            background: Rectangle {
                radius: 26
                color: "#f7f7f7"
                border.color: "#ffffff"
            }
        }

        Item {
            width: Math.max(0, header.width - 52 - header.selectorButtonWidth - (chatList.count > 0 ? 116 : 52) - 56)
            height: 1
        }

        Button {
            visible: chatList.count > 0
            height: 52
            width: 84
            text: "✎    ⋯"
            font.pixelSize: 24
            onClicked: chatController.newChat()
            background: Rectangle {
                radius: 26
                color: "#f7f7f7"
                border.color: "#ffffff"
            }
        }

        IconButton {
            visible: chatList.count === 0
            text: "◌"
            buttonSize: 52
            font.pixelSize: 30
            onClicked: chatController.newChat()
        }
    }

    ListView {
        id: chatList
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: composerWrap.top
        anchors.topMargin: 24
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        clip: true
        spacing: 4
        model: chatController.messages
        delegate: ChatBubble {
            controller: chatController
        }
        onCountChanged: Qt.callLater(positionViewAtEnd)
        onContentHeightChanged: {
            if (chatController.busy)
                Qt.callLater(positionViewAtEnd)
        }
    }

    Item {
        id: emptySpace
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: composerWrap.top
        visible: chatList.count === 0
    }

    Item {
        id: composerWrap
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.bottomMargin: 22
        height: composer.implicitHeight
        z: 3

        Composer {
            id: composer
            width: parent.width
            controller: chatController
            onAddClicked: attachmentMenu.open()
        }
    }

    AttachmentMenu {
        id: attachmentMenu
        parent: Overlay.overlay
        controller: chatController
        store: settingsStore
        onChooserRequested: function(origin) {
            chooserOrigin = origin
            fileDialog.title = origin === "camera" ? "Choose from Camera Roll" :
                               origin === "photos" ? "Choose Photos" : "Choose Files"
            fileDialog.nameFilters = origin === "files" ? ["All files (*)"] :
                                                       ["Images (*.png *.jpg *.jpeg *.heic *.webp)", "All files (*)"]
            fileDialog.open()
        }
    }

    FileDialog {
        id: fileDialog
        fileMode: FileDialog.OpenFiles
        onAccepted: chatController.attachFiles(selectedFiles, chooserOrigin)
    }

    ModelEffortSelector {
        id: selector
        parent: Overlay.overlay
        controller: chatController
        onOpened: {
            if (initialUiState !== "selector-model")
                modelMode = false
        }
    }

    Sidebar {
        id: sidebar
        parent: Overlay.overlay
        controller: chatController
        onSettingsRequested: settingsSheet.open()
    }

    SettingsSheet {
        id: settingsSheet
        parent: Overlay.overlay
        store: settingsStore
    }

    Rectangle {
        id: toast
        visible: chatController.toast.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: composerWrap.top
        anchors.bottomMargin: 14
        width: Math.min(toastText.implicitWidth + 34, parent.width - 40)
        height: toastText.implicitHeight + 20
        radius: 18
        color: "#111111"
        z: 8
        Text {
            id: toastText
            anchors.centerIn: parent
            width: parent.width - 22
            text: chatController.toast
            color: "#ffffff"
            font.pixelSize: 14
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
        Timer {
            running: toast.visible
            repeat: false
            interval: 2600
            onTriggered: chatController.clearToast()
        }
    }

    Timer {
        id: e2eTimer
        interval: 1000
        repeat: true
        onTriggered: {
            if (chatController.busy)
                return

            if (window.e2eStep === 0) {
                window.e2eStep = 1
                chatController.sendMessage("Reply with one short sentence about Qt.")
            } else if (window.e2eStep === 1) {
                window.e2eStep = 2
                chatController.sendMessage("Reply with one short sentence about Markdown.")
            } else {
                stop()
            }
        }
    }

    Component.onCompleted: {
        if (initialUiState === "selector")
            selector.open()
        else if (initialUiState === "selector-model") {
            selector.modelMode = true
            selector.open()
        }
        else if (initialUiState === "sidebar")
            sidebar.open()
        else if (initialUiState === "settings")
            settingsSheet.open()
        else if (initialUiState === "personalization") {
            settingsSheet.open()
            Qt.callLater(function() { settingsSheet.page = "personalization" })
        }
        else if (initialUiState === "attachments")
            attachmentMenu.open()
        else if (initialUiState === "chat-demo")
            chatController.loadDemoConversation()
        else if (initialUiState === "gemma-chat")
            chatController.selectedModel = "Gemma 4 Free"
        else if (initialUiState === "e2e-openrouter") {
            chatController.selectedModel = "Gemma 4 Free"
            e2eTimer.start()
        }
    }
}
