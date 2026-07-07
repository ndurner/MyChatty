import QtQuick

Canvas {
    id: root
    property string iconName
    property color stroke: "#111111"
    property color accent: "#111111"

    width: 30
    height: 30
    antialiasing: true

    onIconNameChanged: requestPaint()
    onStrokeChanged: requestPaint()
    onAccentChanged: requestPaint()

    function roundedRect(ctx, x, y, w, h, r) {
        ctx.beginPath()
        ctx.moveTo(x + r, y)
        ctx.lineTo(x + w - r, y)
        ctx.quadraticCurveTo(x + w, y, x + w, y + r)
        ctx.lineTo(x + w, y + h - r)
        ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
        ctx.lineTo(x + r, y + h)
        ctx.quadraticCurveTo(x, y + h, x, y + h - r)
        ctx.lineTo(x, y + r)
        ctx.quadraticCurveTo(x, y, x + r, y)
    }

    onPaint: {
        var ctx = getContext("2d")
        var dpr = root.Canvas.devicePixelRatio
        ctx.reset()
        ctx.scale(dpr, dpr)
        ctx.lineWidth = 2
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        ctx.strokeStyle = root.stroke
        ctx.fillStyle = root.accent

        if (iconName === "camera") {
            roundedRect(ctx, 4, 8, 22, 16, 5)
            ctx.stroke()
            roundedRect(ctx, 9, 5, 7, 5, 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(15, 16, 4.5, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(22, 12, 1, 0, Math.PI * 2)
            ctx.fill()
        } else if (iconName === "photos") {
            roundedRect(ctx, 7, 5, 17, 19, 4)
            ctx.stroke()
            roundedRect(ctx, 4, 8, 19, 17, 4)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(18.5, 12.5, 1.7, 0, Math.PI * 2)
            ctx.fill()
            ctx.beginPath()
            ctx.moveTo(7.5, 22)
            ctx.lineTo(12, 17)
            ctx.lineTo(15.5, 20.5)
            ctx.lineTo(18.5, 17.5)
            ctx.lineTo(22, 22)
            ctx.stroke()
        } else if (iconName === "files") {
            ctx.beginPath()
            ctx.moveTo(8, 4)
            ctx.lineTo(18, 4)
            ctx.lineTo(24, 10)
            ctx.lineTo(24, 25)
            ctx.lineTo(8, 25)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(18, 4)
            ctx.lineTo(18, 10)
            ctx.lineTo(24, 10)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(12, 15)
            ctx.lineTo(20, 15)
            ctx.moveTo(12, 19)
            ctx.lineTo(20, 19)
            ctx.stroke()
        } else if (iconName === "plugins") {
            roundedRect(ctx, 6, 6, 7, 7, 2.5)
            ctx.stroke()
            roundedRect(ctx, 17, 6, 7, 7, 2.5)
            ctx.stroke()
            roundedRect(ctx, 6, 17, 7, 7, 2.5)
            ctx.stroke()
            roundedRect(ctx, 17, 17, 7, 7, 2.5)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(13, 9.5)
            ctx.lineTo(17, 9.5)
            ctx.moveTo(9.5, 13)
            ctx.lineTo(9.5, 17)
            ctx.moveTo(20.5, 13)
            ctx.lineTo(20.5, 17)
            ctx.moveTo(13, 20.5)
            ctx.lineTo(17, 20.5)
            ctx.stroke()
        } else if (iconName === "webBrowser") {
            roundedRect(ctx, 4, 6, 22, 18, 4)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(4, 11)
            ctx.lineTo(26, 11)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(8, 8.5, 0.8, 0, Math.PI * 2)
            ctx.arc(11, 8.5, 0.8, 0, Math.PI * 2)
            ctx.fill()

            ctx.beginPath()
            ctx.arc(15, 17.5, 5.2, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(9.8, 17.5)
            ctx.lineTo(20.2, 17.5)
            ctx.moveTo(15, 12.3)
            ctx.bezierCurveTo(17, 14.2, 17, 20.8, 15, 22.7)
            ctx.moveTo(15, 12.3)
            ctx.bezierCurveTo(13, 14.2, 13, 20.8, 15, 22.7)
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(20.5, 20.2)
            ctx.lineTo(25.5, 25.2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(25.5, 25.2, 1, 0, Math.PI * 2)
            ctx.fill()
        }
    }
}
