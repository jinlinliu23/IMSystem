import QtQuick

Item {
    width: parent.width
    height: parent.height
    property var n: appWindow.nav
    property var t: appWindow.theme
    property int curTab: 0

    readonly property int notificationCount:
        (typeof ClientFacade !== "undefined") ? ClientFacade.notificationCount : 0

    MessagesPage {
        id: messagesPage
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: tabBar.top
        visible: curTab === 0
    }

    ContactsPage {
        id: contactsPage
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: tabBar.top
        visible: curTab === 1
    }

    ProfilePage {
        id: profilePage
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: tabBar.top
        visible: curTab === 2
    }

    Item {
        id: tabBar
        height: 56
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: t.surface
            border.color: t.border
            border.width: 0.5
        }

        Row {
            anchors.fill: parent

            Repeater {
                model: [
                    { name: "消息", idx: 0, showBadge: false },
                    { name: "联系人", idx: 1, showBadge: true },
                    { name: "我", idx: 2, showBadge: false }
                ]

                Item {
                    width: tabBar.width / 3
                    height: tabBar.height

                    Rectangle {
                        anchors.fill: parent
                        color: tabTap.pressed ? "#e0e0e0" : "transparent"
                    }

                    Text {
                        id: tabLabel
                        anchors.centerIn: parent
                        text: modelData.name
                        font.pixelSize: 16
                        color: curTab === modelData.idx ? "skyblue" : "#999"
                    }

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#e74c3c"
                        visible: modelData.showBadge && notificationCount > 0
                        anchors.left: tabLabel.right
                        anchors.leftMargin: 2
                        anchors.top: tabLabel.top
                        anchors.topMargin: -2
                    }

                    TapHandler {
                        id: tabTap
                        onTapped: curTab = modelData.idx
                    }
                }
            }
        }
    }
}
