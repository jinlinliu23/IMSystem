import QtQuick
import QtQuick.Controls

Item {
    id: addFriendPage

    property var nav: appWindow.nav
    property var theme: appWindow.theme

    property bool hasResult: false
    property int foundUserId: 0
    property string foundAccount: ""
    property string foundNickname: ""

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: theme ? theme.bg : "#f0f0f0"
    }

    // 顶栏
    Item {
        id: header
        height: 48
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: theme ? theme.surface : "#f8f8f8"
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: theme ? theme.border : "#e0e0e0"
            }
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
            // anchors.left: parent.left
            // anchors.top: parent.top
            // anchors.bottom: parent.bottom
            // width: 48
            onTapped: nav.pop()
        }

        Text {
            text: "添加好友"
            font.pixelSize: 18
            font.bold: true
            color: theme ? theme.text : "#333"
            anchors.centerIn: parent
        }
    }

    Column {
        anchors.top: header.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.85
        spacing: 16

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: "#888"
            text: "输入对方账号（精确匹配，区分大小写）"
        }

        Item {
            width: parent.width
            height: 50
            Rectangle {
                anchors.fill: parent
                color: theme ? theme.surface : "#fff"
                border.color: theme ? theme.border : "#ccc"
                border.width: 1
                radius: 8
            }
            TextInput {
                id: accountInput
                anchors.fill: parent
                anchors.margins: 12
                font.pixelSize: 16
                color: theme ? theme.text : "#333"
                verticalAlignment: TextInput.AlignVCenter
            }
            Text {
                text: "对方账号"
                visible: !accountInput.text && !accountInput.activeFocus
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 14
                color: "#aaa"
            }
        }

        Rectangle {
            width: parent.width
            height: 48
            radius: 8
            color: (typeof ClientFacade !== "undefined" && ClientFacade.busy)
                   ? "#9bb8d9" : (theme ? theme.primary : "#4a90d9")

            Text {
                anchors.centerIn: parent
                text: (typeof ClientFacade !== "undefined" && ClientFacade.busy) ? "搜索中..." : "搜索"
                color: "#fff"
                font.pixelSize: 16
                font.bold: true
            }

            TapHandler {
                enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
                onTapped: {
                    hasResult = false
                    if (typeof ClientFacade !== "undefined")
                        ClientFacade.searchUser(accountInput.text)
                }
            }
        }

        // 搜索结果
        Rectangle {
            width: parent.width
            visible: hasResult
            radius: 8
            color: theme ? theme.surface : "#fff"
            border.color: theme ? theme.border : "#e0e0e0"
            implicitHeight: resultCol.implicitHeight + 24

            Column {
                id: resultCol
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: foundNickname || foundAccount
                    font.pixelSize: 18
                    font.bold: true
                    color: theme ? theme.text : "#333"
                }
                Text {
                    text: "账号：" + foundAccount
                    font.pixelSize: 14
                    color: "#666"
                }
                Text {
                    text: "ID：" + foundUserId
                    font.pixelSize: 12
                    color: "#999"
                }

                Rectangle {
                    width: parent.width
                    height: 40
                    radius: 6
                    color: (typeof ClientFacade !== "undefined" && ClientFacade.busy)
                           ? "#9bb8d9" : (theme ? theme.primary : "#4a90d9")

                    Text {
                        anchors.centerIn: parent
                        text: (typeof ClientFacade !== "undefined" && ClientFacade.busy)
                              ? "发送中..." : "发送好友申请"
                        color: "#fff"
                        font.pixelSize: 14
                    }

                    TapHandler {
                        enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
                                 && foundAccount.length > 0
                        onTapped: {
                            if (typeof ClientFacade !== "undefined")
                                ClientFacade.sendFriendRequest(foundAccount)
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: typeof ClientFacade !== "undefined" ? ClientFacade : null
        function onFriendRequestFinished(success, message) {
            appWindow.showAlert(message)
            if (success)
                nav.pop()
        }
        function onSearchUserFinished(success, message, userId, account, nickname) {
            if (!success) {
                hasResult = false
                appWindow.showAlert(message)
                return
            }
            hasResult = true
            foundUserId = userId
            foundAccount = account
            foundNickname = nickname
        }
    }
}
