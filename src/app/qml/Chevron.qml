import QtQuick

Item {
    id: root

    property color color: "#111111"
    property string direction: "right"
    property real strokeWidth: 2.4

    implicitWidth: 18
    implicitHeight: 18

    onColorChanged: canvas.requestPaint()
    onDirectionChanged: canvas.requestPaint()
    onStrokeWidthChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = root.color
            ctx.lineWidth = root.strokeWidth
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.beginPath()

            if (root.direction === "down") {
                ctx.moveTo(width * 0.25, height * 0.38)
                ctx.lineTo(width * 0.5, height * 0.62)
                ctx.lineTo(width * 0.75, height * 0.38)
            } else if (root.direction === "left") {
                ctx.moveTo(width * 0.62, height * 0.25)
                ctx.lineTo(width * 0.38, height * 0.5)
                ctx.lineTo(width * 0.62, height * 0.75)
            } else {
                ctx.moveTo(width * 0.38, height * 0.25)
                ctx.lineTo(width * 0.62, height * 0.5)
                ctx.lineTo(width * 0.38, height * 0.75)
            }

            ctx.stroke()
        }
    }
}
