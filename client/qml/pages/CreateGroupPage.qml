import QtQuick
import QtQuick.Controls

Item {
    id: createGroupPage
    property var nav: appWindow.nav
    property var theme: appWindow.theme

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
            text: "创建群聊"
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
        anchors.bottom: createBtn.top
        anchors.margins: 16
        spacing: 12

        Text {
            text: "群名称"
            font.pixelSize: 13
            color: "#888"
        }

        Rectangle {
            width: parent.width
            height: 44
            radius: 8
            color: theme ? theme.surface : "#fff"
            border.color: theme ? theme.border : "#ccc"

            TextInput {
                id: groupNameInput
                anchors.fill: parent
                anchors.margins: 12
                font.pixelSize: 16
                color: theme ? theme.text : "#333"
                verticalAlignment: TextInput.AlignVCenter
            }
        }

        Text {
            text: "选择好友（至少 1 位，含你共至少 2 人）"
            font.pixelSize: 13
            color: "#888"
        }

        ListView {
            id: friendPicker
            width: parent.width
            height: parent.height - groupNameInput.parent.height - 80
            clip: true
            model: (typeof ClientFacade !== "undefined" && ClientFacade.contacts) ? ClientFacade.contacts : null

            delegate: Item {
                width: friendPicker.width
                height: 52

                property bool checked: selectedAccounts.indexOf(model.account) !== -1

                Rectangle {
                    anchors.fill: parent
                    color: rowTap.pressed ? "#eee" : "transparent"
                }

                Rectangle {
                    width: 22
                    height: 22
                    radius: 4
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    border.color: theme ? theme.primary : "#4a90d9"
                    border.width: 2
                    color: checked ? (theme ? theme.primary : "#4a90d9") : "transparent"

                    Text {
                        visible: checked
                        text: "✓"
                        color: "#fff"
                        font.pixelSize: 14
                        anchors.centerIn: parent
                    }
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 36
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: model.nickname || model.account
                        font.pixelSize: 15
                        color: theme ? theme.text : "#333"
                    }
                    Text {
                        text: model.account
                        font.pixelSize: 12
                        color: "#999"
                    }
                }

                TapHandler {
                    id: rowTap
                    onTapped: {
                        var next = selectedAccounts.slice()
                        var idx = next.indexOf(model.account)
                        if (idx !== -1)
                            next.splice(idx, 1)
                        else
                            next.push(model.account)
                        selectedAccounts = next
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: friendPicker.count === 0
                text: "暂无好友，请先添加好友"
                color: "#999"
                font.pixelSize: 14
            }
        }
    }

    Rectangle {
        id: createBtn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: parent.width * 0.85
        height: 48
        radius: 8
        color: (typeof ClientFacade !== "undefined" && ClientFacade.busy)
               ? "#9bb8d9" : (theme ? theme.primary : "#4a90d9")

        Text {
            anchors.centerIn: parent
            text: (typeof ClientFacade !== "undefined" && ClientFacade.busy) ? "创建中..." : "创建"
            color: "#fff"
            font.pixelSize: 16
            font.bold: true
        }

        TapHandler {
            enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
            onTapped: {
                var members = selectedAccounts.slice()
                if (typeof ClientFacade !== "undefined")
                    ClientFacade.createGroup(groupNameInput.text, members)
            }
        }
    }

    Connections {
        target: typeof ClientFacade !== "undefined" ? ClientFacade : null
        function onCreateGroupFinished(success, message, groupId) {
            appWindow.showAlert(message)
            if (success)
                nav.pop()
        }
    }
}
