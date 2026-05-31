import QtQuick
import client

Item {
    width: parent.width
    height: parent.height
    property var n: appWindow.nav
    property var t: appWindow.theme

    readonly property int pendingRequestCount:
        (typeof ClientFacade !== "undefined") ? ClientFacade.pendingFriendRequestCount : 0
    readonly property int notificationCount:
        (typeof ClientFacade !== "undefined") ? ClientFacade.notificationCount : 0

    Item {
        id: header
        height: 48
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

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
                text: "联系人"
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
                    color: plusTap.pressed ? "#E0E0E0" : "transparent"
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

    Item {
        id: notifyBar
        height: 48
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: t.surface
            border.color: t.border
        }

        Column {
            anchors.fill: parent
            anchors.margins: 8

            Item {
                width: parent.width
                height: 36

                Rectangle {
                    anchors.fill: parent
                    color: friendReqTap.pressed ? "#f0f0f0" : "transparent"
                    radius: 4
                }

                Text {
                    text: "好友申请"
                    font.pixelSize: 15
                    color: t.text
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    width: pendingRequestCount > 9 ? 22 : 18
                    height: 18
                    radius: 9
                    color: "#e74c3c"
                    visible: pendingRequestCount > 0
                    anchors.right: parent.right
                    anchors.rightMargin: 28
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        anchors.centerIn: parent
                        text: pendingRequestCount > 99 ? "99+" : String(pendingRequestCount)
                        color: "#fff"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                Text {
                    text: "›"
                    font.pixelSize: 18
                    color: "#ccc"
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                }

                TapHandler {
                    id: friendReqTap
                    onTapped: n.push("FriendRequestsPage")
                }
            }

        }
    }

    Flickable {
        anchors.top: notifyBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        contentHeight: contentCol.height

        Column {
            id: contentCol
            width: parent.width

            Text {
                width: parent.width
                leftPadding: 16
                topPadding: 10
                text: "群聊"
                font.pixelSize: 13
                color: "#888"
            }

            ListView {
                id: groupList
                width: parent.width
                height: count * 56
                interactive: false
                model: (typeof ClientFacade !== "undefined") ? ClientFacade.groups : null

                delegate: Item {
                    width: groupList.width
                    height: 56

                    Rectangle {
                        anchors.fill: parent
                        color: gTap.pressed ? "lightgrey" : "transparent"
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10
                        ImAvatar {
                            size: 40
                            isGroup: true
                            displayName: model.name
                            seed: "g:" + model.groupId
                            themePrimary: t.primary
                        }
                        Column {
                            Text { text: model.name; font.pixelSize: 16; color: t.text }
                            Text {
                                text: model.memberCount + " 人"
                                font.pixelSize: 12
                                color: "#999"
                            }
                        }
                    }

                    TapHandler {
                        id: gTap
                        onTapped: n.push("GroupInfoPage", {
                            groupId: model.groupId,
                            groupName: model.name
                        })
                    }
                }
            }

            Text {
                width: parent.width
                height: 36
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: groupList.count === 0
                text: "暂无群聊"
                color: "#bbb"
                font.pixelSize: 13
            }

            Text {
                width: parent.width
                leftPadding: 16
                topPadding: 10
                text: "好友"
                font.pixelSize: 13
                color: "#888"
            }

            ListView {
                id: contactList
                width: parent.width
                height: count * 56
                interactive: false
                model: (typeof ClientFacade !== "undefined") ? ClientFacade.contacts : null

                delegate: Item {
                    width: contactList.width
                    height: 56

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10

                        ImAvatar {
                            size: 40
                            displayName: model.nickname
                            seed: model.account
                            themePrimary: t.primary
                        }

                        Column {
                            Text {
                                text: model.nickname || model.account
                                font.pixelSize: 16
                                color: t.text
                            }
                            Text {
                                text: model.account
                                font.pixelSize: 12
                                color: "#999"
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width
                height: 40
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: contactList.count === 0
                text: "暂无好友，点击右上角 + 添加好友"
                color: "#999"
                font.pixelSize: 14
            }
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
        z: 10

        Column {
            anchors.fill: parent
            anchors.margins: 2

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

            Rectangle { width: parent.width; height: 1; color: t.border }

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
                        n.push("CreateGroupPage")
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (typeof ClientFacade !== "undefined")
            ClientFacade.refreshMyGroups()
    }
}
