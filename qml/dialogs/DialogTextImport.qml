import QtQuick 2.0
import QtQuick.Controls 1.4

Item {
    Rectangle {

        color: "#99000000"

        anchors.fill: parent
        MultiPointTouchArea {
            anchors.fill: parent
        }

        Rectangle {
            color: bgColor

            anchors.fill: parent
            anchors.topMargin: portrait ? buttonSize * 2 : 0
            anchors.leftMargin: portrait ? 0 : buttonSize * 2

            TextArea {
                id: mainText
                height: parent.height / 2

                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }

                anchors.margins: buttonSize
                text: ""
                font.pixelSize: fontSize1
                wrapMode: Text.WordWrap
            }

            Row {
                anchors {
                    top: mainText.bottom
                    topMargin: buttonSize
                }

                Rectangle {
                    width: portrait ? buttonSize*1.5 : buttonSize*2
                    height: buttonSize
                    radius: buttonRadius
                    color: bgColor
                    border.width: 1
                    border.color: fgColor

                    Text {
                        anchors.fill: parent
                        text: "import"
                        color: fgColor
                        font.pixelSize: fontSize2
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                    }

                    MultiPointTouchArea {
                        anchors.fill: parent
                        onPressed: {
                            layout.songTextImport.importText(mainText.text);
                        }
                    }
                }

            }
        }
    }
}
