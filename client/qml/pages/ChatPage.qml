import QtQuick
import QtQuick.Controls
import client

Page {
    width: parent.width
    height: parent.height

    property var nav: appWindow.nav
    property var theme: appWindow.theme
    property string target: ""
    property string peerAccount: ""
    property bool isGroup: false
    property int groupId: 0

    readonly property color colorMyBubble: "#4A90D9"
    readonly property color colorMyText: "white"
    readonly property color colorPeerBubble: "#FFFFFF"
    readonly property color colorPeerText: "#333333"
    readonly property color colorInputBg: "#F5F5F5"
    readonly property color colorSendBtn: "#4A90D9"
    // [Feature 2] 待办消息视图项背景色（浅蓝）
    readonly property color colorTodoBubble: "#E3F2FD"
    readonly property int avatarSize: 36
    property bool contextMenuOpen: false

    Component.onCompleted: {
        if (typeof ClientFacade !== "undefined") {
            if (isGroup && groupId > 0)
                ClientFacade.fetchGroupInfo(groupId)
            if (peerAccount.length > 0)
                ClientFacade.openChat(peerAccount, target)
        }
    }

    Item {
        id: headerBar
        height: 48
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: theme ? theme.surface : "#F8F8F8"
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#E0E0E0"
            }
        }

        // [Feature 2] 右上角 "⋮" 更多操作按钮
        Text {
            text: "< 返回"
            font.pixelSize: 16
            color: "#4A90D9"
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            TapHandler { onTapped: nav.pop() }
        }

        Text {
            text: target || peerAccount
            font.pixelSize: 18
            color: "#333333"
            anchors.centerIn: parent
        }

        // [Feature 2] 右上角 "⋮" 更多操作按钮
        Text {
            text: "⋮"
            font.pixelSize: 22
            color: "#4A90D9"
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter

            TapHandler {
                onTapped: {
                    contextMenuOpen = !contextMenuOpen
                }
            }
        }
    }

    // [Feature 2] 下拉菜单：智能标签开关
    Popup {
        id: contextMenu
        visible: contextMenuOpen
        x: parent.width - width - 8
        y: headerBar.height + 4
        width: 200
        padding: 0
        modal: false
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        onClosed: contextMenuOpen = false

        contentItem: Column {
            width: parent.width

            ItemDelegate {
                width: parent.width
                height: 42
                contentItem: Row {
                    spacing: 10
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12

                    Text {
                        text: typeof ClientFacade !== "undefined" && ClientFacade.smartTagEnabled
                              ? "关闭智能标签识别"
                              : "开启智能标签识别"
                        font.pixelSize: 14
                        color: "#333333"
                    }
                }

                TapHandler {
                    onTapped: {
                        if (typeof ClientFacade !== "undefined") {
                            ClientFacade.setSmartTagEnabled(!ClientFacade.smartTagEnabled)
                        }
                        contextMenu.close()
                    }
                }
            }
        }
    }

    ListView {
        id: messageList
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: inputBar.top
        anchors.bottomMargin: 8
        clip: true
        spacing: 12
        model: (typeof ClientFacade !== "undefined") ? ClientFacade.messages : null

        delegate: Item {
            id: msgRow
            width: messageList.width
            readonly property bool isMine: model.who === "me"
            // [Feature 2] 判断该消息是否被分类器判定为待办
            readonly property bool isTodo: typeof ClientFacade !== "undefined"
                                          && ClientFacade.smartTagEnabled
                                          && (model.todoScore || 0) > 0.55
            readonly property bool showSenderLabel: isGroup && !isMine && (model.senderName || "").length > 0
            readonly property string avatarNick: isMine
                ? ((typeof ClientFacade !== "undefined" && ClientFacade.currentUser)
                   ? ClientFacade.currentUser.nickname : "")
                : ((model.senderName || "").length > 0 ? model.senderName : target)
            readonly property string avatarSeed: isMine
                ? ((typeof ClientFacade !== "undefined" && ClientFacade.currentUser)
                   ? ClientFacade.currentUser.account : "")
                : ((model.senderAccount || "").length > 0 ? model.senderAccount : peerAccount)

            height: rowLayout.height + (showSenderLabel ? 20 : 0) + 4

            // [Feature 2] 待办消息 → 整行背景浅蓝
            Rectangle {
                anchors.fill: parent
                radius: 6
                color: isTodo ? colorTodoBubble : "transparent"
            }

            Text {
                id: senderLabel
                text: model.senderName || ""
                font.pixelSize: 12
                color: "#888"
                visible: showSenderLabel
                anchors.left: parent.left
                anchors.leftMargin: 16 + avatarSize + 8
                anchors.top: parent.top
                anchors.topMargin: 2
            }

            Row {
                id: rowLayout
                anchors.top: showSenderLabel ? senderLabel.bottom : parent.top
                anchors.topMargin: showSenderLabel ? 4 : 0
                anchors.left: isMine ? undefined : parent.left
                anchors.right: isMine ? parent.right : undefined
                anchors.leftMargin: isMine ? 0 : 12
                anchors.rightMargin: isMine ? 12 : 0
                spacing: 8

                ImAvatar {
                    visible: !isMine
                    size: avatarSize
                    displayName: avatarNick
                    seed: avatarSeed
                    themePrimary: theme ? theme.primary : "#4A90D9"
                }

                Rectangle {
                    id: bubble
                    width: Math.min(msgText.paintedWidth + 24, messageList.width * 0.65)
                    height: msgText.paintedHeight + 16
                    radius: 8
                    color: isMine ? colorMyBubble : colorPeerBubble
                    border.color: isMine ? "transparent" : "#E0E0E0"
                    border.width: isMine ? 0 : 1

                    Text {
                        id: msgText
                        text: model.text
                        font.pixelSize: 15
                        color: isMine ? colorMyText : colorPeerText
                        wrapMode: Text.WordWrap
                        width: Math.min(messageList.width * 0.65 - 24, implicitWidth)
                        anchors.centerIn: parent
                    }
                }

                ImAvatar {
                    visible: isMine
                    size: avatarSize
                    displayName: avatarNick
                    seed: avatarSeed
                    themePrimary: theme ? theme.primary : "#4A90D9"
                }
            }
        }

        onCountChanged: if (count > 0) positionViewAtEnd()
    }

    Item {
        id: inputBar
        height: 56
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: theme ? theme.surface : "#F8F8F8"
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: "#E0E0E0"
            }
        }

        Row {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Rectangle {
                width: parent.width - sendButton.width - 10
                height: 40
                radius: 10
                color: colorInputBg
                border.color: "#DCDCDC"
                border.width: 1

                TextInput {
                    id: messageInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 15
                    color: "#333333"
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                }
            }

            Rectangle {
                id: sendButton
                width: 70
                height: 40
                radius: 6
                color: (typeof ClientFacade !== "undefined" && ClientFacade.busy)
                       ? "#9bb8d9" : colorSendBtn

                Text {
                    anchors.centerIn: parent
                    text: (typeof ClientFacade !== "undefined" && ClientFacade.busy) ? "..." : "发送"
                    color: "white"
                    font.pixelSize: 15
                    font.bold: true
                }

                TapHandler {
                    enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
                    onTapped: {
                        var msg = messageInput.text.trim()
                        if (msg !== "" && typeof ClientFacade !== "undefined") {
                            if (isGroup)
                                ClientFacade.sendGroupMessage(groupId, msg)
                            else
                                ClientFacade.sendPrivateMessage(msg)
                            messageInput.text = ""
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: typeof ClientFacade !== "undefined" ? ClientFacade : null
        function onSendMessageFinished(success, message) {
            if (!success)
                appWindow.showAlert(message)
        }
        function onChatHistoryLoaded() {
            if (messageList.count > 0)
                messageList.positionViewAtEnd()
        }
        function onGroupInfoLoaded(success, message, gid, name, members) {
            if (!success)
                return
            if (isGroup && gid === groupId)
                target = name.length > 0 ? name : target
        }
    }
}
