import QtQuick
import QtQuick.Controls

Item {
    id: root
    required property int index
    required property bool isUser
    required property bool isAssistant
    required property string text
    required property string html
    required property var blocks
    required property string reasoning
    required property var attachments
    required property bool streaming
    required property var toolCalls
    required property bool toolOnly
    required property var approval
    property var controller
    signal editRequested(int row, string text)

    function hasUsefulReasoning(value) {
        var text = (value || "").trim()
        return text.length > 0 && text !== "Thought"
    }

    width: ListView.view ? ListView.view.width : 400
    visible: !root.toolOnly
    implicitHeight: root.toolOnly ? 0 : bubbleColumn.implicitHeight + 18

    Column {
        id: bubbleColumn
        width: parent.width - 32
        x: 16
        y: 8
        spacing: 12

        Text {
            visible: root.index === 0
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Today"
            color: "#999999"
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }

        Flow {
            visible: attachments.length > 0
            width: parent.width
            spacing: 8
            Repeater {
                model: attachments
                delegate: Rectangle {
                    width: modelData.isImage ? 112 : Math.min(fileName.implicitWidth + 28, parent.width)
                    height: modelData.isImage ? 112 : 38
                    radius: 18
                    color: "#eeeeee"
                    border.color: "#dddddd"
                    clip: true
                    Image {
                        visible: modelData.isImage
                        anchors.fill: parent
                        anchors.margins: 2
                        source: modelData.url
                        fillMode: Image.PreserveAspectCrop
                    }
                    Text {
                        id: fileName
                        visible: !modelData.isImage
                        anchors.centerIn: parent
                        width: parent.width - 20
                        text: modelData.fileName
                        elide: Text.ElideMiddle
                        color: "#333333"
                        font.pixelSize: 13
                    }
                }
            }
        }

        Item {
            id: userMessageWrap
            visible: root.isUser
            width: parent.width
            implicitHeight: userBubble.height + (userHover.hovered ? userActions.height + 6 : 0)

            HoverHandler {
                id: userHover
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                gesturePolicy: TapHandler.WithinBounds
                onLongPressed: userMenu.popup()
            }

            Menu {
                id: userMenu
                MenuItem {
                    text: "Copy"
                    onTriggered: controller.copyMessage(root.index)
                }
                MenuItem {
                    text: "Edit"
                    enabled: !controller.busy
                    onTriggered: root.editRequested(root.index, root.text)
                }
            }

            Rectangle {
                id: userBubble
                anchors.right: parent.right
                width: Math.min(userText.implicitWidth + 42, parent.width * 0.78)
                height: userText.implicitHeight + 30
                radius: 26
                color: "#000000"

                TextEdit {
                    id: userText
                    anchors.fill: parent
                    anchors.margins: 17
                    text: root.text
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.Wrap
                    readOnly: true
                    selectByMouse: true
                    selectByKeyboard: true
                    color: "#ffffff"
                    font.pixelSize: 20
                }
            }

            Row {
                id: userActions
                visible: userHover.hovered || userMenu.visible
                anchors.top: userBubble.bottom
                anchors.topMargin: 6
                anchors.right: userBubble.right
                height: 30
                spacing: 8

                Button {
                    width: 58
                    height: 30
                    text: "Copy"
                    padding: 0
                    onClicked: controller.copyMessage(root.index)
                    contentItem: Text {
                        text: parent.text
                        color: "#666666"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 8; color: "#f1f1f1" }
                }

                Button {
                    width: 54
                    height: 30
                    text: "Edit"
                    padding: 0
                    enabled: !controller.busy
                    onClicked: root.editRequested(root.index, root.text)
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#666666" : "#aaaaaa"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 8; color: "#f1f1f1" }
                }
            }
        }

        Column {
            visible: root.isAssistant
            width: parent.width
            spacing: 12

            TextEdit {
                visible: root.hasUsefulReasoning(reasoning)
                width: parent.width
                text: reasoning.trim()
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.WordWrap
                readOnly: true
                selectByMouse: true
                selectByKeyboard: true
                color: "#9b9b9b"
                font.pixelSize: 19
            }

            Column {
                visible: root.toolCalls.length > 0
                width: parent.width
                spacing: 8
                Repeater {
                    model: root.toolCalls
                    delegate: Rectangle {
                        id: toolCard
                        width: parent.width
                        implicitHeight: toolHeader.height + (expanded ? detailWrap.implicitHeight + 10 : 0)
                        height: implicitHeight
                        radius: 8
                        color: "#eef1f5"
                        border.color: "#dce2ea"
                        property bool expanded: false
                        clip: true

                        Column {
                            anchors.fill: parent
                            anchors.margins: 0

                            Button {
                                id: toolHeader
                                width: parent.width
                                height: 42
                                padding: 0
                                onClicked: toolCard.expanded = !toolCard.expanded
                                background: Rectangle {
                                    color: toolHeader.down ? "#dde4ee" : "transparent"
                                }
                                contentItem: Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8
                                    Text {
                                        text: modelData.isOutput ? "✓" : "⚒"
                                        color: "#627084"
                                        font.pixelSize: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        width: parent.width - 44
                                        text: modelData.isOutput ? modelData.label : "Used tool " + modelData.label + (modelData.hasOutput ? " · result" : "")
                                        color: "#3d4756"
                                        font.pixelSize: 15
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: toolCard.expanded ? "▴" : "▾"
                                        color: "#7b8795"
                                        font.pixelSize: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }

                            Rectangle {
                                id: detailWrap
                                visible: toolCard.expanded
                                width: parent.width - 14
                                x: 7
                                implicitHeight: toolDetailsColumn.implicitHeight + 18
                                radius: 6
                                color: "#dfe5ee"
                                Column {
                                    id: toolDetailsColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 9
                                    spacing: 10
                                    Column {
                                        visible: !modelData.isOutput
                                        width: parent.width
                                        spacing: 4
                                        Text {
                                            width: parent.width
                                            text: modelData.label + " arguments"
                                            color: "#526070"
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                        TextEdit {
                                            width: parent.width
                                            text: modelData.arguments
                                            textFormat: TextEdit.PlainText
                                            wrapMode: TextEdit.Wrap
                                            readOnly: true
                                            selectByMouse: true
                                            selectByKeyboard: true
                                            color: "#26313f"
                                            font.family: "Menlo"
                                            font.pixelSize: 12
                                        }
                                    }
                                    Column {
                                        visible: modelData.hasOutput || modelData.isOutput
                                        width: parent.width
                                        spacing: 4
                                        Text {
                                            width: parent.width
                                            text: "Result"
                                            color: "#526070"
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                        TextEdit {
                                            width: parent.width
                                            text: modelData.output
                                            textFormat: TextEdit.PlainText
                                            wrapMode: TextEdit.Wrap
                                            readOnly: true
                                            selectByMouse: true
                                            selectByKeyboard: true
                                            color: "#26313f"
                                            font.family: "Menlo"
                                            font.pixelSize: 12
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                visible: root.approval && root.approval.type === "open_web_page"
                width: parent.width
                implicitHeight: approvalColumn.implicitHeight + 20
                radius: 8
                color: "#fff4d8"
                border.color: "#ead79c"

                Column {
                    id: approvalColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 10

                    Text {
                        width: parent.width
                        text: root.approval.status === "pending"
                              ? "Allow Web Browser to open this URL?"
                              : "Web Browser access " + root.approval.status
                        color: "#4c3a00"
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        wrapMode: Text.Wrap
                    }

                    TextEdit {
                        width: parent.width
                        text: root.approval.url || ""
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.Wrap
                        readOnly: true
                        selectByMouse: true
                        selectByKeyboard: true
                        color: "#2d260f"
                        font.pixelSize: 13
                    }

                    Row {
                        visible: root.approval.status === "pending"
                        spacing: 8
                        Button {
                            text: "Yes"
                            onClicked: controller.resolveToolApproval(root.index, "yes")
                            background: Rectangle { radius: 14; color: "#ffffff"; border.color: "#e0cc8f" }
                        }
                        Button {
                            text: "No"
                            onClicked: controller.resolveToolApproval(root.index, "no")
                            background: Rectangle { radius: 14; color: "#ffffff"; border.color: "#e0cc8f" }
                        }
                        Button {
                            text: "Always"
                            onClicked: controller.resolveToolApproval(root.index, "always")
                            background: Rectangle { radius: 14; color: "#ffffff"; border.color: "#e0cc8f" }
                        }
                    }
                }
            }

            Column {
                width: parent.width
                spacing: 10
                Repeater {
                    model: root.blocks
                    delegate: Loader {
                        width: parent.width
                        property real contentHeight: item ? item.implicitHeight : 0
                        height: contentHeight
                        sourceComponent: {
                            if (modelData.type === "heading")
                                return headingComponent
                            if (modelData.type === "code")
                                return codeComponent
                            if (modelData.type === "rule")
                                return ruleComponent
                            if (modelData.type === "list")
                                return listComponent
                            if (modelData.type === "table")
                                return tableComponent
                            if (modelData.type === "quote")
                                return quoteComponent
                            return paragraphComponent
                        }
                        property var block: modelData
                        onLoaded: item.block = block
                    }
                }
            }

            Row {
                spacing: 14
                visible: root.text.length > 0 && !root.streaming
                Button {
                    text: "Copy"
                    onClicked: controller.copyMessage(root.index)
                    contentItem: Text {
                        text: parent.text
                        color: "#666666"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 14; color: "#f1f1f1" }
                }
                Button {
                    text: "Read"
                    onClicked: controller.readAloud(root.index)
                    contentItem: Text {
                        text: parent.text
                        color: "#666666"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 14; color: "#f1f1f1" }
                }
                Button {
                    text: "Share"
                    onClicked: controller.shareMessage(root.index)
                    contentItem: Text {
                        text: parent.text
                        color: "#666666"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 14; color: "#f1f1f1" }
                }
                Button {
                    text: "Fork"
                    onClicked: controller.forkConversation(root.index)
                    contentItem: Text {
                        text: parent.text
                        color: "#666666"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 14; color: "#f1f1f1" }
                }
            }
        }
    }

    Component {
        id: paragraphComponent
        TextEdit {
            property var block
            width: parent.width
            text: block.html
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.WordWrap
            readOnly: true
            selectByMouse: true
            selectByKeyboard: true
            color: "#111111"
            font.pixelSize: 21
        }
    }

    Component {
        id: headingComponent
        TextEdit {
            property var block
            width: parent.width
            text: block.html
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.WordWrap
            readOnly: true
            selectByMouse: true
            selectByKeyboard: true
            color: "#111111"
            font.pixelSize: Math.max(20, 29 - block.level * 2)
            font.weight: Font.Bold
        }
    }

    Component {
        id: codeComponent
        Rectangle {
            property var block
            width: parent.width
            implicitHeight: codeText.implicitHeight + 24
            height: implicitHeight
            radius: 12
            color: "#f4f4f4"
            border.color: "#e5e5e5"
            clip: true
            TextEdit {
                id: codeText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                text: block.text
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                selectByKeyboard: true
                color: "#111111"
                font.family: "Menlo"
                font.pixelSize: 16
            }
        }
    }

    Component {
        id: ruleComponent
        Rectangle {
            property var block
            width: parent.width
            implicitHeight: 1
            height: implicitHeight
            color: "#dddddd"
        }
    }

    Component {
        id: quoteComponent
        Item {
            property var block
            width: parent.width
            implicitHeight: quoteText.implicitHeight
            Rectangle {
                x: 0
                width: 3
                height: quoteText.implicitHeight
                radius: 2
                color: "#dddddd"
            }
            TextEdit {
                id: quoteText
                x: 13
                width: parent.width - 13
                text: block.html
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.WordWrap
                readOnly: true
                selectByMouse: true
                selectByKeyboard: true
                color: "#555555"
                font.pixelSize: 20
            }
        }
    }

    Component {
        id: listComponent
        Column {
            property var block
            width: parent.width
            spacing: 6
            function ordinal(row, level) {
                var count = 0
                for (var i = 0; i <= row; ++i) {
                    var item = block.items[i]
                    if (item && item.ordered && (item.level || 0) === (level || 0))
                        count += 1
                }
                return count
            }
            Repeater {
                model: block.items
                delegate: Row {
                    x: Math.min(48, (modelData.level || 0) * 22)
                    width: parent.width - x
                    spacing: 8
                    Text {
                        width: 24
                        text: modelData.ordered ? ordinal(index, modelData.level) + "." : "•"
                        color: "#111111"
                        font.pixelSize: 20
                        horizontalAlignment: Text.AlignRight
                    }
                    TextEdit {
                        width: parent.width - 32
                        text: modelData.html
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.WordWrap
                        readOnly: true
                        selectByMouse: true
                        selectByKeyboard: true
                        color: "#111111"
                        font.pixelSize: 21
                    }
                }
            }
        }
    }

    Component {
        id: tableComponent
        Rectangle {
            property var block
            width: parent.width
            implicitHeight: tableColumn.implicitHeight
            height: implicitHeight
            radius: 10
            border.color: "#e2e2e2"
            color: "#ffffff"
            clip: true

            Column {
                id: tableColumn
                width: parent.width

                Row {
                    width: parent.width
                    Repeater {
                        model: block.headers
                        delegate: Rectangle {
                            width: tableColumn.width / Math.max(1, block.headers.length)
                            height: Math.max(40, headerText.implicitHeight + 18)
                            color: "#f1f1f1"
                            border.color: "#e2e2e2"
                            TextEdit {
                                id: headerText
                                anchors.fill: parent
                                anchors.margins: 8
                                text: modelData.html
                                textFormat: TextEdit.RichText
                                wrapMode: TextEdit.WordWrap
                                readOnly: true
                                selectByMouse: true
                                selectByKeyboard: true
                                font.pixelSize: 15
                                font.weight: Font.Bold
                                color: "#111111"
                            }
                        }
                    }
                }

                Repeater {
                    model: block.rows
                    delegate: Row {
                        width: tableColumn.width
                        property var rowData: modelData
                        Repeater {
                            model: rowData.cells
                            delegate: Rectangle {
                                width: tableColumn.width / Math.max(1, block.headers.length)
                                height: Math.max(38, cellText.implicitHeight + 16)
                                color: "#ffffff"
                                border.color: "#e9e9e9"
                                TextEdit {
                                    id: cellText
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    text: modelData.html
                                    textFormat: TextEdit.RichText
                                    wrapMode: TextEdit.WordWrap
                                    readOnly: true
                                    selectByMouse: true
                                    selectByKeyboard: true
                                    font.pixelSize: 15
                                    color: "#111111"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
