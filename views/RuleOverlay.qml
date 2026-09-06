import QtQuick

Item {
    id: root

    function mapPoint(point) {
        return VisionController.frameToItem(point.x, point.y, width, height)
    }

    function strokeColor(kind) {
        if (kind === 1)
            return "#F7B731"
        if (kind === 2)
            return "#FA8231"
        if (kind === 3)
            return "#20BF6B"
        return "#74D4FF"
    }

    function paintGeometry(ctx, kind, points, enabled, isDraft) {
        ctx.reset()
        if (!points || points.length < 1 || width <= 0 || height <= 0)
            return
        if (VisionController.frameWidth <= 0 || VisionController.frameHeight <= 0)
            return

        ctx.globalAlpha = enabled ? 1 : 0.35
        ctx.strokeStyle = isDraft ? "#FFFFFF" : strokeColor(kind)
        ctx.lineWidth = 2
        ctx.fillStyle = (kind === 0 || kind === 2) ? (isDraft ? "#33FFFFFF" : "#3374D4FF") : "transparent"

        const first = mapPoint(points[0])
        ctx.beginPath()
        ctx.moveTo(first.x, first.y)
        for (let i = 1; i < points.length; ++i) {
            const mapped = mapPoint(points[i])
            ctx.lineTo(mapped.x, mapped.y)
        }
        if (kind === 0 || kind === 2)
            ctx.closePath()
        if (kind === 0 || kind === 2)
            ctx.fill()
        ctx.stroke()

        if ((kind === 1 || kind === 3) && points.length >= 2)
            drawArrow(ctx, mapPoint(points[0]), mapPoint(points[1]))

        if (isDraft) {
            ctx.fillStyle = "#FFFFFF"
            for (let i = 0; i < points.length; ++i) {
                const vertex = mapPoint(points[i])
                ctx.beginPath()
                ctx.arc(vertex.x, vertex.y, 3, 0, Math.PI * 2)
                ctx.fill()
            }
        }
    }

    function drawArrow(ctx, from, to) {
        const dx = to.x - from.x
        const dy = to.y - from.y
        const length = Math.hypot(dx, dy)
        if (length < 1)
            return
        const ux = dx / length
        const uy = dy / length
        const size = 10
        ctx.beginPath()
        ctx.moveTo(to.x, to.y)
        ctx.lineTo(to.x - ux * size - uy * size * 0.5, to.y - uy * size + ux * size * 0.5)
        ctx.lineTo(to.x - ux * size + uy * size * 0.5, to.y - uy * size - ux * size * 0.5)
        ctx.closePath()
        ctx.fillStyle = ctx.strokeStyle
        ctx.fill()
    }

    Repeater {
        model: VisionController.ruleModel

        Canvas {
            required property int kind
            required property var points
            required property bool enabled

            anchors.fill: parent
            antialiasing: true
            onPaint: root.paintGeometry(getContext("2d"), kind, points, enabled, false)
            onPointsChanged: requestPaint()
            onEnabledChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            Connections {
                target: VisionController
                function onFrameSizeChanged() { requestPaint() }
            }
        }
    }

    Canvas {
        id: draftCanvas
        anchors.fill: parent
        antialiasing: true
        onPaint: root.paintGeometry(getContext("2d"), VisionController.drawTool === 3 ? 2
                                      : VisionController.drawTool === 4 ? 3
                                      : VisionController.drawTool === 2 ? 1
                                      : 0,
                                      VisionController.draftPoints, true, true)
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        Connections {
            target: VisionController
            function onDraftPointsChanged() { draftCanvas.requestPaint() }
            function onDrawToolChanged() { draftCanvas.requestPaint() }
            function onFrameSizeChanged() { draftCanvas.requestPaint() }
        }
    }
}
