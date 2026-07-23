import QtQuick
import QtQuick.Controls

Item {
    id: fieldRoot
    property string label
    property string value: ""
    signal edited(string value)

    onValueChanged: {
        if (input.text !== value)
            input.text = value
    }

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
            // TextEdited misses some iOS text insertion paths, notably paste
            // and credential autofill. Keep the backing settings value in sync
            // for every change while avoiding a feedback loop on reload.
            onTextChanged: {
                if (text !== fieldRoot.value)
                    fieldRoot.edited(text)
            }
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
