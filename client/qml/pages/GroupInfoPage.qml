import QtQuick
import QtQuick.Controls

Item {
    id: groupInfoPage
    property var nav: appWindow.nav
    property var theme: appWindow.theme

    property int groupId: 0
    property string groupName: ""
    property var members: []
    property bool canManage: true

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
        TapHandler { onTapped: nav.pop() }

        Text {
            text: "群资料"
            font.pixelSize: 18
            font.bold: true
            color: theme ? theme.text : "#333"
            anchors.centerIn: parent
        }
    }

    Column {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        spacing: 12

        Rectangle {
            width: parent.width
            height: 72
            radius: 8
            color: theme ? theme.surface : "#fff"
            border.color: theme ? theme.border : "#e0e0e0"

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Text {
                    text: groupName || "加载中..."
                    font.pixelSize: 18
                    font.bold: true
                    color: theme ? theme.text : "#333"
                }
                Text {
                    text: "群 ID：" + groupId
                    font.pixelSize: 12
                    color: "#999"
                }
            }
        }

        Row {
            width: parent.width
            spacing: 8

            Text {
                text: "群成员（" + members.length + "）"
                font.pixelSize: 14
                color: "#666"
            }

            Item { width: 1; height: 1 }

            Rectangle {
                visible: canManage
                width: 88
                height: 28
                radius: 6
                color: theme ? theme.primary : "#4a90d9"
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: "邀请成员"
                    color: "#fff"
                    font.pixelSize: 12
                }
                TapHandler {
                    onTapped: nav.push("InviteMemberPage", { groupId: groupId, groupName: groupName })
                }
            }

            Rectangle {
                visible: canManage
                width: 72
                height: 28
                radius: 6
                color: "#e74c3c"
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: "退群"
                    color: "#fff"
                    font.pixelSize: 12
                }
                TapHandler {
                    onTapped: {
                        if (typeof ClientFacade !== "undefined")
                            ClientFacade.leaveGroup(groupId)
                    }
                }
            }
        }

        ListView {
            width: parent.width
            height: parent.height - 120
            clip: true
            model: members

            delegate: Item {
                width: parent.width
                height: 56

                Rectangle {
                    anchors.fill: parent
                    color: theme ? theme.surface : "#fff"
                    border.color: theme ? theme.border : "#eee"
                    radius: 6
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: modelData.nickname || modelData.account
                        font.pixelSize: 15
                        color: theme ? theme.text : "#333"
                    }
                    Text {
                        text: modelData.account + (modelData.role === "owner" ? " · 群主" : "")
                        font.pixelSize: 12
                        color: "#999"
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (typeof ClientFacade !== "undefined" && groupId > 0)
            ClientFacade.fetchGroupInfo(groupId)
    }

    Connections {
        target: typeof ClientFacade !== "undefined" ? ClientFacade : null
        function onGroupInfoLoaded(success, message, gid, name, memberList) {
            if (gid !== groupId)
                return
            if (!success) {
                appWindow.showAlert(message)
                return
            }
            groupName = name
            members = memberList
        }
        function onLeaveGroupFinished(success, message, dissolved) {
            appWindow.showAlert(message)
            if (success && !dissolved) {
                nav.pop()
            }
            if (success && dissolved) {
                nav.pop()
            }
        }
        function onFriendNotify(message) {
            if (message && message.length > 0)
                appWindow.showAlert(message)
        }
    }
}
