import QtQuick
import QtQuick.Controls
import client

Item {
    id: invitePage
    property var nav: appWindow.nav
    property var theme: appWindow.theme
    property int groupId: 0
    property string groupName: ""
    property var selectedAccounts: []

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
        Rectangle { anchors.fill: parent; color: theme ? theme.surface : "#f8f8f8" }
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
            text: "邀请成员"
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
        anchors.bottom: inviteBtn.top
        anchors.margins: 16
        spacing: 10

        Text { text: "群：" + groupName; font.pixelSize: 14; color: "#666" }
        Text { text: "从好友中选择要邀请的成员"; font.pixelSize: 13; color: "#888" }

        ListView {
            id: friendList
            width: parent.width
            height: parent.height - 80
            clip: true
            model: (typeof ClientFacade !== "undefined") ? ClientFacade.contacts : null

            delegate: Item {
                width: friendList.width
                height: 54
                property bool checked: selectedAccounts.indexOf(model.account) !== -1
                Rectangle { anchors.fill: parent; color: tap.pressed ? "#eee" : "transparent" }
                Rectangle {
                    width: 22; height: 22; radius: 4
                    anchors.left: parent.left; anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    border.color: theme ? theme.primary : "#4a90d9"
                    border.width: 2
                    color: checked ? (theme ? theme.primary : "#4a90d9") : "transparent"
                    Text { visible: checked; text: "✓"; color: "#fff"; font.pixelSize: 14; anchors.centerIn: parent }
                }
                Column {
                    anchors.left: parent.left; anchors.leftMargin: 36
                    anchors.verticalCenter: parent.verticalCenter
                    Text { text: model.nickname || model.account; font.pixelSize: 15; color: theme ? theme.text : "#333" }
                    Text { text: model.account; font.pixelSize: 12; color: "#999" }
                }
                TapHandler {
                    id: tap
                    onTapped: {
                        var next = selectedAccounts.slice()
                        var idx = next.indexOf(model.account)
                        if (idx !== -1) next.splice(idx, 1)
                        else next.push(model.account)
                        selectedAccounts = next
                    }
                }
            }
            Text {
                anchors.centerIn: parent
                visible: friendList.count === 0
                text: "暂无好友"
                color: "#999"
                font.pixelSize: 14
            }
        }
    }

    Rectangle {
        id: inviteBtn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: parent.width * 0.85
        height: 48
        radius: 8
        color: (typeof ClientFacade !== "undefined" && ClientFacade.busy) ? "#9bb8d9" : (theme ? theme.primary : "#4a90d9")
        Text { anchors.centerIn: parent; text: "邀请"; color: "#fff"; font.pixelSize: 16; font.bold: true }
        TapHandler {
            enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
            onTapped: {
                if (typeof ClientFacade !== "undefined")
                    ClientFacade.inviteGroupMembers(groupId, selectedAccounts)
            }
        }
    }

    Connections {
        target: typeof ClientFacade !== "undefined" ? ClientFacade : null
        function onInviteMembersFinished(success, message) {
            appWindow.showAlert(message)
            if (success) nav.pop()
        }
    }
}
