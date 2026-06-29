import QtQuick
import QtQuick.Controls

Item {
    id: fieldRoot
    property string label
    property alias text: input.text
    signal edited()

    height: 84
    Column {
        anchors.fill: parent
        spacing: 8
        Text {
            width: parent.width
            text: fieldRoot.label
            color: "#666666"
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
        TextField {
            id: input
            width: parent.width
            height: 52
            echoMode: TextInput.Password
            placeholderText: "sk-..."
            font.pixelSize: 15
            color: "#111111"
            placeholderTextColor: "#9a9a9a"
            selectedTextColor: "#ffffff"
            selectionColor: "#111111"
            onTextEdited: fieldRoot.edited()
            leftPadding: 14
            rightPadding: 14
            background: Rectangle {
                radius: 14
                color: "#ffffff"
                border.color: input.activeFocus ? "#111111" : "#d9d9d9"
                border.width: input.activeFocus ? 2 : 1
            }
        }
    }
}
