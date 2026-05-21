import QtQuick

Item {
    anchors.fill: parent
    property var n: ApplicationWindow.window.nav
    property var t: ApplicationWindow.window.theme
    property int curTab: 0

    // 内容区
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

    // 底部 TabBar
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
                    { name: "消息", idx: 0 },
                    { name: "联系人", idx: 1 },
                    { name: "我", idx: 2 }
                ]

                Item {
                    width: tabBar.width / 3
                    height: tabBar.height

                    // 点击背景反馈
                    Rectangle {
                        anchors.fill: parent
                        color: tabTap.pressed ? "#e0e0e0" : "transparent"
                    }

                    Text {
                        anchors.centerIn: parent
                        text: modelData.name
                        font.pixelSize: 16
                        color: curTab === modelData.idx ? "skyblue" : "#999"
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