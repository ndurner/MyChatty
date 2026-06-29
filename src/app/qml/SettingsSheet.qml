import QtQuick
import QtQuick.Controls

Popup {
    id: root
    property var store
    property string page: "settings"

    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: parent ? parent.width : 430
    height: parent ? parent.height : 900
    x: 0
    y: 0
    padding: 0
    onOpened: page = "settings"

    background: Rectangle {
        color: "#efeff3"
    }

    Item {
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 70
            color: "#f5f5f8"
            radius: 34
        }

        Column {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 86
            anchors.bottomMargin: 24
            spacing: 24

            Row {
                width: parent.width
                height: 56
                Button {
                    width: 56
                    height: 56
                    text: page === "settings" ? "×" : "‹"
                    font.pixelSize: 24
                    onClicked: {
                        if (page === "settings")
                            root.close()
                        else
                            page = "settings"
                    }
                    background: Rectangle {
                        radius: 28
                        color: "#ffffff"
                    }
                }
                Text {
                    width: parent.width - 152
                    height: parent.height
                    text: page === "settings" ? "Settings" : "Personalization"
                    font.pixelSize: 23
                    font.weight: Font.Bold
                    color: "#111111"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                Button {
                    width: 96
                    height: 56
                    text: "Save"
                    font.pixelSize: 20
                    enabled: true
                    onClicked: {
                        store.save()
                        if (page === "personalization")
                            page = "settings"
                        else
                            root.close()
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#111111" : "#aaaaaa"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 28
                        color: "#ffffff"
                    }
                }
            }

            Column {
                visible: page === "settings"
                width: parent.width
                spacing: 24

                Text {
                    text: "Providers"
                    color: "#969696"
                    font.pixelSize: 21
                    font.weight: Font.Bold
                    leftPadding: 18
                }
                Rectangle {
                    width: parent.width
                    height: 232
                    radius: 26
                    color: "#ffffff"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12
                        LabeledSecretField {
                            width: parent.width
                            label: "OpenAI API Key"
                            text: store.openAIKey
                            onEdited: store.openAIKey = text
                        }
                        LabeledSecretField {
                            width: parent.width
                            label: "OpenRouter API Key"
                            text: store.openRouterKey
                            onEdited: store.openRouterKey = text
                        }
                    }
                }

                Text {
                    text: "Customize"
                    color: "#969696"
                    font.pixelSize: 21
                    font.weight: Font.Bold
                    leftPadding: 18
                }
                Button {
                    width: parent.width
                    height: 76
                    onClicked: page = "personalization"
                    background: Rectangle {
                        radius: 26
                        color: "#ffffff"
                    }
                    contentItem: Item {
                        anchors.fill: parent
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 22
                            anchors.right: disclosure.left
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Personalization"
                            color: "#111111"
                            font.pixelSize: 23
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        Item {
                            id: disclosure
                            anchors.right: parent.right
                            anchors.rightMargin: 22
                            anchors.verticalCenter: parent.verticalCenter
                            width: 32
                            height: parent.height

                            Chevron {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                direction: "right"
                                color: "#bbbbbb"
                                strokeWidth: 2.3
                            }
                        }
                    }
                }
            }

            Column {
                visible: page === "personalization"
                width: parent.width
                spacing: 16
                Text {
                    text: "Custom instructions"
                    color: "#969696"
                    font.pixelSize: 21
                    font.weight: Font.Bold
                    leftPadding: 18
                }
                Rectangle {
                    width: parent.width
                    height: 190
                    radius: 26
                    color: "#ffffff"
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: 18
                        text: store.customInstructions
                        wrapMode: TextEdit.Wrap
                        font.pixelSize: 20
                        color: "#111111"
                        background: null
                        onTextChanged: store.customInstructions = text
                    }
                }
            }
        }
    }
}
