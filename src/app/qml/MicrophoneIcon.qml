import QtQuick

Item {
    id: root

    property color color: "#111111"
    property real strokeWidth: 2.2

    implicitWidth: 22
    implicitHeight: 22

    onColorChanged: canvas.requestPaint()
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
            ctx.roundedRect(width * 0.36, height * 0.12, width * 0.28, height * 0.48, width * 0.14, width * 0.14)
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(width * 0.22, height * 0.46)
            ctx.bezierCurveTo(width * 0.22, height * 0.72, width * 0.78, height * 0.72, width * 0.78, height * 0.46)
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(width * 0.5, height * 0.72)
            ctx.lineTo(width * 0.5, height * 0.88)
            ctx.moveTo(width * 0.35, height * 0.88)
            ctx.lineTo(width * 0.65, height * 0.88)
            ctx.stroke()
        }
    }
}
