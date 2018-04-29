import QtQuick 2.0

Item {
    property var model;

    GridView {
        model: parent.model

        anchors.fill: parent
        cellWidth: buttonSize * 2
        cellHeight: buttonSize

        flow: GridView.FlowLeftToRight

        delegate: Rectangle {
            color: bgColor
            width: buttonSize * 2
            height: buttonSize
            radius: buttonRadius
            border.width: borderWidth
            border.color: modelData.selected ? hlColor: borderColor

            Text {
                anchors.fill: parent
                text: modelData.text
                font.pixelSize: fontSize1
                color: modelData.selected ? hlColor : fgColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
            }

            MultiPointTouchArea {
                anchors.fill: parent

                onCanceled: {
                    modelData.onReleased()
                }

                onReleased: {
                    modelData.onReleased()
                }
            }
        }
    }
}

