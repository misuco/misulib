import QtQuick 2.0

Item {
    id: root

    property var model

    Repeater {
        model: root.model

        delegate: Item {
            property int rad: root.height * modelData.age / 2

            Rectangle {
                visible: modelData.visible
                x: modelData.x - rad
                y: modelData.y - rad
                radius: rad
                width: rad * 2
                height: rad * 2
                color: "Transparent"
                border.width: 2
                border.color: Qt.hsla(modelData.hue,0.8,0.6,1)
            }
        }
    }
}

