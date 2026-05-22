import QtQuick
import QtQuick.Controls

Page {
    width: parent.width
    height: parent.height

    property var nav: appWindow.nav
    property var theme: appWindow.theme
    property string target: ""
    property string peerAccount: ""

    readonly property color colorMyBubble: "#4A90D9"
    readonly property color colorMyText: "white"
    readonly property color colorPeerBubble: "#FFFFFF"
    readonly property color colorPeerText: "#333333"
    readonly property color colorInputBg: "#F5F5F5"
    readonly property color colorSendBtn: "#4A90D9"

    Component.onCompleted: {
        if (typeof ClientFacade !== "undefined" && peerAccount.length > 0)
            ClientFacade.openChat(peerAccount, target)
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
            width: messageList.width
            height: Math.max(msgText.paintedHeight + 18, 36)

            Rectangle {
                id: bubble
                anchors.right: model.who === "me" ? parent.right : undefined
                anchors.left: model.who !== "me" ? parent.left : undefined
                anchors.rightMargin: model.who === "me" ? 12 : 0
                anchors.leftMargin: model.who !== "me" ? 12 : 0
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(msgText.paintedWidth + 24, messageList.width * 0.75)
                height: msgText.paintedHeight + 16
                radius: 8
                color: model.who === "me" ? colorMyBubble : colorPeerBubble
                border.color: model.who === "me" ? "transparent" : "#E0E0E0"
                border.width: model.who === "me" ? 0 : 1
            }

            Text {
                id: msgText
                text: model.text
                font.pixelSize: 15
                color: model.who === "me" ? colorMyText : colorPeerText
                wrapMode: Text.WordWrap
                width: Math.min(messageList.width * 0.75 - 24, implicitWidth)
                anchors.centerIn: bubble
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
    }
}
