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
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: t.surface
            border.color: t.border
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "联系人"
            font.pixelSize: 18
            color: t.text
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Item {
                width: 36
                height: 32

                Text {
                    id: requestIcon
                    anchors.centerIn: parent
                    text: "👤"
                    font.pixelSize: 18
                }

                Rectangle {
                    width: pendingRequestCount > 9 ? 18 : 16
                    height: 16
                    radius: 8
                    color: "#e74c3c"
                    visible: pendingRequestCount > 0
                    anchors.right: parent.right
                    anchors.top: parent.top

                    Text {
                        anchors.centerIn: parent
                        text: pendingRequestCount > 99 ? "99+" : String(pendingRequestCount)
                        color: "#fff"
                        font.pixelSize: 9
                        font.bold: true
                    }
                }

                TapHandler {
                    onTapped: n.push("FriendRequestsPage")
                }
            }

            Rectangle {
                width: 88
                height: 32
                radius: 6
                color: t.primary

                Text {
                    anchors.centerIn: parent
                    text: "添加好友"
                    color: "#fff"
                    font.pixelSize: 13
                }

                TapHandler {
                    onTapped: n.push("AddFriendPage")
                }
            }
        }
    }

    ListView {
        id: contactList
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        clip: true
        model: (typeof ClientFacade !== "undefined") ? ClientFacade.contacts : null

        delegate: Item {
            width: contactList.width
            height: 56

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
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

        Text {
            anchors.centerIn: parent
            visible: contactList.count === 0
            text: "暂无好友，点击右上角添加好友"
            color: "#999"
            font.pixelSize: 14
        }
    }

}
