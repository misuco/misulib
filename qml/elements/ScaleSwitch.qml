import QtQuick 2.0

Item {
    property var chordLevel: ["ii","II","iii","III","IV","v","V","vi","VI","vii","VII"];
    GridView {
        model: layout.ScaleSwitch

        anchors.fill: parent
        cellWidth: buttonSize
        cellHeight: buttonSize

        flow: GridView.FlowLeftToRight
        interactive: false

        delegate: Rectangle {
            color: modelData.selected ? layout.pitchColors[modelData.rootNote].colorOn
                                      : layout.pitchColors[modelData.rootNote].colorOff
            width: buttonSize
            height: buttonSize
            radius: buttonRadius
            border.width: borderWidth
            border.color: modelData.selected ? hlColor: borderColor

            Text {
                anchors.fill: parent
                text: chordLevel[index]
                font.pixelSize: fontSize1
                color: modelData.selected ? hlColor : fgColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
            }

            Text {
                anchors.fill: parent
                text: modelData.text1 + modelData.text2
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

