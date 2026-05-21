import QtQuick

Item {
    anchors.fill: parent
    property var t: ApplicationWindow.window.theme

    Text {
        anchors.centerIn: parent
        text: "联系人"
        font.pixelSize: 20
        color: "#999"
    }
}