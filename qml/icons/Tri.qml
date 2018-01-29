import QtQuick 2.0

Item {
    property int xStep: width / 4
    property int yStep: height / 2

    property var bgColorHl: modelData.selected ? hlColor : bgColor
    property int strokeWidth: 1+height/50

    Rectangle {
        anchors.fill: parent
        color: bgColorHl
    }

    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.lineWidth = strokeWidth
            ctx.strokeStyle = fgColor
            ctx.fillStyle = fgColor

            ctx.beginPath()
            ctx.moveTo(0,yStep)
            ctx.lineTo(xStep, 0)
            ctx.lineTo(xStep * 2, yStep)
            ctx.lineTo(xStep * 3, yStep * 2)
            ctx.lineTo(xStep * 4, yStep)

            ctx.stroke()
        }
    }
}
