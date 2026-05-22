import QtQuick
import QtQuick.Controls

Item {
    id: friendRequestsPage
    property var nav: appWindow.nav
    property var theme: appWindow.theme

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: theme ? theme.bg : "#f0f0f0"
    }

    Item {
        id: header
        height: 48
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: theme ? theme.surface : "#f8f8f8"
        }

        Text {
            text: "‹"
            font.pixelSize: 28
            color: theme ? theme.primary : "#4a90d9"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
        }
        TapHandler {
            onTapped: nav.pop()
        }

        Text {
            text: "好友申请"
            font.pixelSize: 18
            font.bold: true
            color: theme ? theme.text : "#333"
            anchors.centerIn: parent
        }

        TapHandler {
            onTapped: {
                if (typeof ClientFacade !== "undefined")
                    ClientFacade.refreshFriendRequests()
            }
        }
        Text {
            text: "刷新"
            font.pixelSize: 14
            color: theme ? theme.primary : "#4a90d9"
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    ListView {
        id: requestList
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        clip: true
        spacing: 8
        model: (typeof ClientFacade !== "undefined") ? ClientFacade.friendRequests : null

        delegate: Item {
            width: requestList.width
            height: 72

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: theme ? theme.surface : "#fff"
                border.color: theme ? theme.border : "#e0e0e0"
            }

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Text {
                    text: model.fromNickname || model.fromAccount
                    font.pixelSize: 16
                    font.bold: true
                    color: theme ? theme.text : "#333"
                }
                Text {
                    text: "账号：" + model.fromAccount
                    font.pixelSize: 13
                    color: "#888"
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Rectangle {
                    width: 56
                    height: 32
                    radius: 6
                    color: theme ? theme.surface : "#f5f5f5"
                    border.color: theme ? theme.border : "#ccc"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "拒绝"
                        color: "#666"
                        font.pixelSize: 14
                    }

                    TapHandler {
                        enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
                        onTapped: ClientFacade.rejectFriendRequest(model.fromAccount)
                    }
                }

                Rectangle {
                    width: 56
                    height: 32
                    radius: 6
                    color: theme ? theme.primary : "#4a90d9"

                    Text {
                        anchors.centerIn: parent
                        text: "同意"
                        color: "#fff"
                        font.pixelSize: 14
                    }

                    TapHandler {
                        enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
                        onTapped: ClientFacade.acceptFriendRequest(model.fromAccount)
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: requestList.count === 0
            text: "暂无好友申请"
            color: "#999"
            font.pixelSize: 15
        }
    }

    Connections {
        target: typeof ClientFacade !== "undefined" ? ClientFacade : null
        function onAcceptFriendFinished(success, message) {
            appWindow.showAlert(message)
            if (success && typeof ClientFacade !== "undefined")
                ClientFacade.syncAfterLogin()
        }
        function onRejectFriendFinished(success, message) {
            appWindow.showAlert(message)
            if (success && typeof ClientFacade !== "undefined")
                ClientFacade.refreshFriendRequests()
        }
    }

    Component.onCompleted: {
        if (typeof ClientFacade !== "undefined")
            ClientFacade.refreshFriendRequests()
    }
}
