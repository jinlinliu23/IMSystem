import QtQuick
import QtQuick.Controls

Item {
    id: registerPage

    property var nav: appWindow.nav
    property var theme: appWindow.theme

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: theme ? theme.bg : "#f0f0f0"
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: formColumn.height + 40
        clip: true

        Column {
            id: formColumn
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 24
            width: parent.width * 0.8
            spacing: 16

            Text {
                text: "注册"
                font.pixelSize: 32
                font.bold: true
                color: theme ? theme.text : "#333"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#888"
                text: "真机请填电脑局域网 IP（如 192.168.1.5）；模拟器用 10.0.2.2"
            }

            // 服务器 IP
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
                    id: serverHostInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 16
                    color: theme ? theme.text : "#333"
                    clip: true
                    verticalAlignment: TextInput.AlignVCenter
                    inputMethodHints: Qt.ImhNoPredictiveText
                }

                Text {
                    text: "服务器 IP"
                    visible: !serverHostInput.text && !serverHostInput.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    color: "#aaa"
                    font.pixelSize: 16
                }
            }

            // 服务器端口
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
                    id: serverPortInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 16
                    color: theme ? theme.text : "#333"
                    clip: true
                    verticalAlignment: TextInput.AlignVCenter
                    inputMethodHints: Qt.ImhDigitsOnly
                }

                Text {
                    text: "端口 (16701)"
                    visible: !serverPortInput.text && !serverPortInput.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    color: "#aaa"
                    font.pixelSize: 16
                }
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
                    id: nicknameInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 16
                    color: theme ? theme.text : "#333"
                    clip: true
                    verticalAlignment: TextInput.AlignVCenter
                }

                Text {
                    text: "昵称（展示名，可日后修改）"
                    visible: !nicknameInput.text && !nicknameInput.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    color: "#aaa"
                    font.pixelSize: 14
                }
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
                    clip: true
                    verticalAlignment: TextInput.AlignVCenter
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                }

                Text {
                    text: "账号（唯一，类似微信号）"
                    visible: !accountInput.text && !accountInput.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    color: "#aaa"
                    font.pixelSize: 14
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                color: "#999"
                text: "账号用于登录，全站唯一且区分大小写（Alice 与 alice 不同）；若已被占用请换一个"
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
                    id: passwordInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 16
                    color: theme ? theme.text : "#333"
                    echoMode: TextInput.Password
                    clip: true
                    verticalAlignment: TextInput.AlignVCenter
                }

                Text {
                    text: "密码"
                    visible: !passwordInput.text && !passwordInput.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    color: "#aaa"
                    font.pixelSize: 16
                }
            }

            Rectangle {
                width: parent.width
                height: 50
                radius: 8
                color: (typeof ClientFacade !== "undefined" && ClientFacade.busy)
                       ? "#9bb8d9"
                       : (theme ? theme.primary : "#4a90d9")

                Text {
                    anchors.centerIn: parent
                    text: (typeof ClientFacade !== "undefined" && ClientFacade.busy) ? "注册中..." : "注册"
                    color: "#fff"
                    font.pixelSize: 18
                    font.bold: true
                }

                TapHandler {
                    enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
                    onTapped: {
                        if (typeof ClientFacade === "undefined") {
                            appWindow.showAlert("ClientFacade 未初始化")
                            return
                        }
                        applyServerSettings()
                    ClientFacade.registerUser(
                        nicknameInput.text,
                        accountInput.text,
                        passwordInput.text
                    )
                    }
                }
            }

            Text {
                text: "已有账号？去登录"
                color: theme ? theme.primary : "#4a90d9"
                font.pixelSize: 14
                anchors.horizontalCenter: parent.horizontalCenter

                TapHandler {
                    onTapped: nav.pop()
                }
            }
        }
    }

    function applyServerSettings() {
        if (typeof ClientFacade === "undefined")
            return
        ClientFacade.serverHost = serverHostInput.text.trim()
        var port = parseInt(serverPortInput.text, 10)
        if (!isNaN(port) && port > 0)
            ClientFacade.serverPort = port
        ClientFacade.saveServerSettings()
    }

    Component.onCompleted: {
        if (typeof ClientFacade === "undefined")
            return
        serverHostInput.text = ClientFacade.serverHost
        serverPortInput.text = String(ClientFacade.serverPort)
    }

    Connections {
        target: typeof ClientFacade !== "undefined" ? ClientFacade : null
        function onRegisterFinished(success, message, userId) {
            if (success) {
                nav.pop()
                return
            }
            appWindow.showAlert(message)
        }
    }
}
