import QtQuick 2.0

Item {
    id: root
    property var model
    property var pitchColors

    GridView {
        model: root.model

        anchors.fill: parent
        cellWidth: buttonSize
        cellHeight: buttonSize

        flow: GridView.FlowLeftToRight

        delegate: Rectangle {
            color: modelData.selected ? root.pitchColors[modelData.rootNote].colorOn
                                      : root.pitchColors[modelData.rootNote].colorOff
            width: buttonSize
            height: buttonSize
            radius: buttonRadius
            border.width: borderWidth
            border.color: modelData.selected ? hlColor: borderColor

            Text {
                anchors.fill: parent
                text: modelData.text1
                font.pixelSize: fontSize1
                color: modelData.selected ? hlColor : fgColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
            }

            Text {
                anchors.fill: parent
                text: modelData.text2
                font.pixelSize: fontSize3
                color: modelData.selected ? hlColor : fgColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
            }

            MultiPointTouchArea {
                anchors.fill: parent

                onPressed: {
                    for(var i=0;i<touchPoints.length;i++) {
                        modelData.onPressed()
                    }
                }

                onCanceled: {
                    for(var i=0;i<touchPoints.length;i++) {
                        modelData.onReleased()
                    }
                }

                onReleased: {
                    for(var i=0;i<touchPoints.length;i++) {
                        modelData.onReleased()
                    }
                }
            }
        }
    }
}
