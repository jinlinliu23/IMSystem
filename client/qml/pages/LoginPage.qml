import QtQuick
import QtQuick.Controls

Item {
    id: loginPage

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
                text: "登录"
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
                text: "真机请填电脑 IP；与注册页使用同一服务器地址"
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
                    id: serverHostInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 16
                    color: theme ? theme.text : "#333"
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
                    id: serverPortInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 16
                    color: theme ? theme.text : "#333"
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
                    verticalAlignment: TextInput.AlignVCenter
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                }
                Text {
                    text: "账号"
                    visible: !accountInput.text && !accountInput.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    color: "#aaa"
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
                    id: passwordInput
                    anchors.fill: parent
                    anchors.margins: 12
                    font.pixelSize: 16
                    color: theme ? theme.text : "#333"
                    echoMode: TextInput.Password
                    verticalAlignment: TextInput.AlignVCenter
                }
                Text {
                    text: "密码"
                    visible: !passwordInput.text && !passwordInput.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    color: "#aaa"
                }
            }

            Rectangle {
                width: parent.width
                height: 50
                radius: 8
                color: (typeof ClientFacade !== "undefined" && ClientFacade.busy)
                       ? "#9bb8d9" : (theme ? theme.primary : "#4a90d9")
                Text {
                    anchors.centerIn: parent
                    text: (typeof ClientFacade !== "undefined" && ClientFacade.busy) ? "登录中..." : "登录"
                    color: "#fff"
                    font.pixelSize: 18
                    font.bold: true
                }
                TapHandler {
                    enabled: typeof ClientFacade !== "undefined" && !ClientFacade.busy
                    onTapped: {
                        if (typeof ClientFacade === "undefined")
                            return
                        applyServerSettings()
                        ClientFacade.loginUser(accountInput.text, passwordInput.text)
                    }
                }
            }

            Text {
                text: "没有账号？去注册"
                color: theme ? theme.primary : "#4a90d9"
                font.pixelSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
                TapHandler { onTapped: nav.push("RegisterPage") }
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
        function onLoginFinished(success, message) {
            if (success) {
                nav.replace("MainPage")
                return
            }
            appWindow.showAlert(message)
        }
        function onFriendNotify(message) {
            appWindow.showAlert(message)
        }
    }
}
