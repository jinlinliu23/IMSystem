import QtQuick

Item {
    anchors.fill: parent
    property var n: appWindow.nav
    property var t: appWindow.theme

    // 顶部菜单栏
    Item {
        id: header
        height: 48
        anchors { top: parent.top; left: parent.left; right: parent.right }

        Rectangle {
            anchors.fill: parent
            color: t.surface
            border.color: t.border
        }

        Row {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12

            // 标题，占据剩余空间
            Text {
                text: "消息"
                font.pixelSize: 18
                color: t.text
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - plusBtn.width - 8
            }

            // 加号按钮
            Item {
                id: plusBtn
                width: 36
                height: 36
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.fill: parent
                    radius: 18
                    color: plusTap.pressed ?  "#e0e0e0": "white"
                }

                Text {
                    text: "+"
                    font.pixelSize: 15
                    color: t.primary
                    anchors.centerIn: parent
                }

                TapHandler {
                    id: plusTap
                    onTapped: contextMenu.visible = !contextMenu.visible
                }
            }
        }
    }

    // 消息列表
    ListView {
        anchors.top: header.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        model: ListModel {
            ListElement { name: "回射测试"; last: "点击进入回声测试"; time: "刚刚" }
        }

        delegate: Item {
            width: ListView.view.width
            height: 64

            Rectangle {
                anchors.fill: parent
                color: listTap.pressed ? "lightgrey" : "transparent"
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text {
                    text: model.name
                    font.pixelSize: 16
                    color: t.text
                }
                Text {
                    text: model.last
                    font.pixelSize: 13
                    color: "#999"
                }
            }

            Text {
                text: model.time
                font.pixelSize: 12
                color: "#aaa"
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
            }

            TapHandler {
                id: listTap
                onTapped: n.push("ChatPage", { target: model.name })
            }
        }
    }


    // 上下文菜单（弹出位置相对整个 header）
    Rectangle {
        id: contextMenu
        visible: false
        width: 120
        height: 80
        radius: 6
        color: t.surface
        border.color: t.border
        //z: 10
        anchors.right: header.right
        anchors.rightMargin: 12
        anchors.top: header.bottom
        anchors.topMargin: 4

        Column {
            anchors.fill: parent
            anchors.margins: 2

            Item {
                width: parent.width
                height: parent.height / 2
                Rectangle {
                    anchors.fill: parent
                    color: createGroupTap.pressed ?  "grey": "white"
                    radius: 4
                }
                Text {
                    text: "创建群聊"
                    color: t.text
                    font.pixelSize: 14
                    anchors.centerIn: parent
                }
                TapHandler {
                    id: createGroupTap
                    gesturePolicy: TapHandler.WithinBounds
                    onTapped: {
                        contextMenu.visible = false;
                        console.log("创建群聊");
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: t.border
            }

            Item {
                width: parent.width
                height: parent.height / 2
                Rectangle {
                    anchors.fill: parent
                    color: addFriendTap.pressed ?  "grey": "white"
                    radius: 4
                }
                Text {
                    text: "添加好友"
                    color: t.text
                    font.pixelSize: 14
                    anchors.centerIn: parent
                }
                TapHandler {
                    id: addFriendTap
                    gesturePolicy: TapHandler.WithinBounds
                    onTapped: {
                        contextMenu.visible = false;
                        console.log("添加好友");
                    }
                }
            }
        }
    }
}