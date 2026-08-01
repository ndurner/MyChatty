import QtQuick
import QtQuick.Controls

TextEdit {
    id: control

    persistentSelection: true

    Menu {
        id: selectionMenu

        MenuItem {
            text: "Copy"
            enabled: control.selectedText.length > 0
            onTriggered: control.copy()
        }

        MenuItem {
            text: "Select All"
            onTriggered: control.selectAll()
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        acceptedDevices: PointerDevice.TouchScreen
        gesturePolicy: TapHandler.DragThreshold

        onLongPressed: {
            const position = control.positionAt(point.position.x, point.position.y)
            control.forceActiveFocus(Qt.MouseFocusReason)
            control.cursorPosition = position
            control.selectWord()

            // TextEdit's selectByMouse deliberately excludes touchscreens.
            // Starting the selection here lets iOS expose draggable handles;
            // the menu also makes clipboard access immediate and predictable.
            selectionMenu.popup(control, point.position)
        }
    }
}
