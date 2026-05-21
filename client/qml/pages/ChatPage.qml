import QtQuick
import QtQuick.Controls
Page {
    anchors.fill: parent

    // ---- 快捷引用全局对象 ----
    property var nav: appWindow.nav
    property var theme: appWindow.theme

    // ---- 外部传入的会话标题 ----
    property string target: ""

    // ---- 颜色常量（不依赖主题的文字 / 气泡颜色） ----
    readonly property color colorMyBubble: "#4A90D9"      // 我方气泡背景
    readonly property color colorMyText: "white"           // 我方文字颜色
    readonly property color colorPeerBubble: "#FFFFFF"     // 对方气泡背景
    readonly property color colorPeerText: "#333333"       // 对方文字颜色
    readonly property color colorInputBg: "#F5F5F5"        // 输入框背景
    readonly property color colorSendBtn: "#4A90D9"        // 发送按钮色

    // ---- 临时本地消息模型（后续替换为 C++ 模型） ----
    ListModel {
        id: msgModel
        // 示例数据
        ListElement { who: "me"; text: "你好，回声测试" }
        ListElement { who: "echo"; text: "你好，回声测试 —— 来自服务器" }
    }

    // ---- 顶部标题栏 ----
    Item {
        id: headerBar
        height: 48
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: theme ? theme.surface : "#F8F8F8"
            // 底部细线
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#E0E0E0"
            }
        }

        // 返回按钮
        Text {
            text: "< 返回"
            font.pixelSize: 16
            color: "#4A90D9"
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter

            TapHandler {
                onTapped: nav.pop()
            }
        }

        // 会话标题
        Text {
            text: target
            font.pixelSize: 18
            color: "#333333"
            anchors.centerIn: parent
        }
    }

    // ---- 消息列表区域 ----
    ListView {
        id: messageList
        //height: 500
        //height: Qt.inputMethod.visible ? 500: 800
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: inputBar.top
        anchors.bottomMargin: 8
        clip: true
        spacing: 12
        //verticalLayoutDirection: ListView.BottomToTop

        model: msgModel

        delegate: Item {
            width: messageList.width
            height: Math.max(msgText.paintedHeight + 18, 36)

            // 气泡背景
            Rectangle {
                id: bubble
                // 根据发送者决定气泡的对齐方向
                anchors.right: who === "me" ? parent.right : undefined
                anchors.left: who !== "me" ? parent.left : undefined
                anchors.rightMargin: who === "me" ? 12 : 0
                anchors.leftMargin: who !== "me" ? 12 : 0
                anchors.verticalCenter: parent.verticalCenter

                // 动态宽度：文字宽度 + 内边距
                width: Math.min(msgText.paintedWidth + 24, messageList.width * 0.75)
                height: msgText.paintedHeight + 16
                radius: 8

                color: who === "me" ? colorMyBubble : colorPeerBubble
                border.color: who === "me" ? "transparent" : "#E0E0E0"
                border.width: who === "me" ? 0 : 1
            }

            // 气泡内的文字（避免错位：直接居中在气泡内）
            Text {
                id: msgText
                text: model.text
                font.pixelSize: 15
                color: who === "me" ? colorMyText : colorPeerText
                wrapMode: Text.WordWrap
                // 最大宽度 = 列表宽度的 75% - 内外边距
                width: Math.min(messageList.width * 0.75 - 24, implicitWidth)
                anchors.centerIn: bubble       // 关键：锚定到气泡中心，不会错位
            }
        }

        // 自动滚动到底部（当有新消息时）
        onCountChanged: {
            currentIndex = count - 1;
        }
    }

    // ---- 底部输入区域 ----
    Item {
        id: inputBar
        height: 56
        //anchors.top: messageList.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        //height: Qt.inputMethod.visible ? 150: 56
        // anchors.bottomMargin: Qt.inputMethod.visible
        //                      ? Qt.inputMethod.keyboardRectangle.height
        //                      : 0
        Rectangle {
            anchors.fill: parent
            color: theme ? theme.surface : "#F8F8F8"
            // 顶部分割线
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
            anchors.topMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            // 输入框
            Rectangle {
                id: inputBg
                width: parent.width - sendButton.width - 10
                height: 40
                radius: 10
                color: colorInputBg
                border.color: "#DCDCDC"
                border.width: 1

                TextInput {
                    id: messageInput
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    font.pixelSize: 15
                    color: "#333333"
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                    activeFocusOnPress: true
                }
            }

            // 发送按钮
            Rectangle {
                id: sendButton
                width: 70
                height: 40
                radius: 6          // 适当圆角
                color: colorSendBtn

                Text {
                    text: "发送"
                    color: "white"
                    font.pixelSize: 15
                    font.bold: true
                    anchors.centerIn: parent
                }

                TapHandler {
                    onTapped: {
                        let msg = messageInput.text.trim();
                        if (msg !== "") {
                            // 添加到本地模型（后续改为调用 ClientFacade.sendEcho）
                            msgModel.append({ who: "me", text: msg });
                            messageInput.text = "";

                            // 模拟回射回复（后续删除）
                            echoTimer.echo = msg;
                            echoTimer.start();
                        }
                    }
                }
            }
        }


    }

    // ---- 临时回射定时器（后续删除） ----
    Timer {
        id: echoTimer
        interval: 500
        repeat: false
        property string echo: ""
        onTriggered: {
            msgModel.append({ who: "echo", text: echo + " —— 来自服务器" });
        }
    }
}