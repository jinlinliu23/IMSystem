import QtQuick

Item {
    width: parent.width
    height: parent.height
    property var n: appWindow.nav
    property var t: appWindow.theme
    property int curTab: 0

    readonly property int notificationCount:
        (typeof ClientFacade !== "undefined") ? ClientFacade.notificationCount : 0
    readonly property int unreadMessageCount:
        (typeof ClientFacade !== "undefined") ? ClientFacade.unreadMessageCount : 0

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
                    { name: "消息", idx: 0, badgeCount: unreadMessageCount },
                    { name: "联系人", idx: 1, badgeCount: notificationCount },
                    { name: "我", idx: 2, badgeCount: 0 }
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
                        width: modelData.badgeCount > 9 ? 18 : (modelData.badgeCount > 0 ? 8 : 0)
                        height: modelData.badgeCount > 9 ? 18 : 8
                        radius: modelData.badgeCount > 9 ? 9 : 4
                        color: "#e74c3c"
                        visible: modelData.badgeCount > 0
                        anchors.left: tabLabel.right
                        anchors.leftMargin: 2
                        anchors.top: tabLabel.top
                        anchors.topMargin: modelData.badgeCount > 9 ? -6 : -2

                        Text {
                            anchors.centerIn: parent
                            visible: modelData.badgeCount > 9
                            text: modelData.badgeCount > 99 ? "99+" : String(modelData.badgeCount)
                            color: "#fff"
                            font.pixelSize: 9
                            font.bold: true
                        }
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
