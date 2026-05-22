import QtQuick

Item {
    width: parent.width
    height: parent.height
    property var n: appWindow.nav
    property var t: appWindow.theme

    readonly property int pendingRequestCount:
        (typeof ClientFacade !== "undefined") ? ClientFacade.pendingFriendRequestCount : 0

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

            Text {
                text: "消息"
                font.pixelSize: 18
                color: t.text
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - plusBtn.width - 8
            }

            Item {
                id: plusBtn
                width: 36
                height: 36
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.fill: parent
                    radius: 18
                    color: plusTap.pressed ? "#e0e0e0" : "white"
                }

                Text {
                    text: "+"
                    font.pixelSize: 15
                    color: t.primary
                    anchors.centerIn: parent
                }

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: "#e74c3c"
                    visible: pendingRequestCount > 0
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 2
                    anchors.topMargin: 2
                }

                TapHandler {
                    id: plusTap
                    onTapped: contextMenu.visible = !contextMenu.visible
                }
            }
        }
    }

    ListView {
        id: conversationList
        anchors.top: header.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        model: (typeof ClientFacade !== "undefined") ? ClientFacade.conversations : null

        delegate: Item {
            width: conversationList.width
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
                visible: model.time.length > 0
            }

            TapHandler {
                id: listTap
                onTapped: n.push("ChatPage", {
                    target: model.name,
                    peerAccount: model.peerAccount || model.conversationId
                })
            }
        }

        Text {
            anchors.centerIn: parent
            visible: conversationList.count === 0
            text: "暂无会话，添加好友后将出现在这里"
            color: "#999"
            font.pixelSize: 14
        }
    }

    Rectangle {
        id: contextMenu
        visible: false
        width: 120
        height: 80
        radius: 6
        color: t.surface
        border.color: t.border
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
                    color: createGroupTap.pressed ? "grey" : "white"
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
                        contextMenu.visible = false
                        console.log("创建群聊")
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
                    color: addFriendTap.pressed ? "grey" : "white"
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
                        contextMenu.visible = false
                        n.push("AddFriendPage")
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (typeof ClientFacade !== "undefined")
            ClientFacade.syncAfterLogin()
    }
}
