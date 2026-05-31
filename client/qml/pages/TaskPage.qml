import QtQuick
import QtQuick.Controls
import client

Page {
    width: parent.width
    height: parent.height

    property var nav: appWindow.nav
    property var theme: appWindow.theme
    property bool detailOpen: false
    property string detailText: ""
    property string detailSender: ""
    property string detailTime: ""

    function refreshTasks() {
        if (typeof ClientFacade !== "undefined")
            ClientFacade.reloadTasks()
    }

    function formatTime(ts) {
        if (!ts || ts <= 0) return ""
        var d = new Date(ts * 1000)
        var m = d.getMonth() + 1
        var day = d.getDate()
        var h = d.getHours()
        var min = d.getMinutes()
        if (min < 10) min = "0" + min
        return m + "/" + day + " " + h + ":" + min
    }

    function showDetail(text, sender, ts) {
        detailText = text
        detailSender = sender
        detailTime = formatTime(ts)
        detailOpen = true
    }

    Component.onCompleted: refreshTasks()

    Rectangle {
        anchors.fill: parent
        color: "#F2F2F2"
    }

    Item {
        id: headerBar
        height: 48
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
            anchors.fill: parent
            color: "#FFFFFF"
        }

        Text {
            text: "事务"
            font.pixelSize: 18
            color: "#333333"
            anchors.centerIn: parent
        }
    }

    ListView {
        id: taskList
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        clip: true
        spacing: 8
        model: (typeof ClientFacade !== "undefined") ? ClientFacade.tasks : null

        delegate: Rectangle {
            width: taskList.width
            height: Math.max(60, col.height + 16)
            radius: 8
            color: model.isCompleted ? "#F5F5F5" : "#FFFFFF"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                Rectangle {
                    width: 22
                    height: 22
                    radius: 3
                    border.color: "#CCCCCC"
                    border.width: 1
                    color: model.isCompleted ? "#4A90D9" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: model.isCompleted ? "✓" : ""
                        color: "white"
                        font.pixelSize: 14
                    }

                    TapHandler {
                        onTapped: {
                            if (typeof ClientFacade !== "undefined")
                                ClientFacade.tasks.toggleCompleted(model.taskId, !model.isCompleted)
                        }
                    }
                }

                Column {
                    id: col
                    width: taskList.width - 90
                    spacing: 6
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: model.originalText
                        font.pixelSize: 14
                        color: model.isCompleted ? "#AAAAAA" : "#333333"
                        wrapMode: Text.WordWrap
                        width: parent.width
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        font.strikeout: model.isCompleted
                    }

                    Text {
                        text: "查看原消息  >"
                        font.pixelSize: 12
                        color: "#4A90D9"

                        TapHandler {
                            onTapped: showDetail(model.originalText,
                                                 model.fromNickname || "",
                                                 model.createdAt || 0)
                        }
                    }
                }
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.top: parent.top
                anchors.topMargin: 8
                text: "✕"
                font.pixelSize: 12
                color: "#CCCCCC"

                TapHandler {
                    onTapped: {
                        if (typeof ClientFacade !== "undefined")
                            ClientFacade.tasks.removeTask(model.taskId)
                    }
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: "暂无待办事项\n长按聊天消息可设为代办"
        font.pixelSize: 15
        color: "#CCCCCC"
        horizontalAlignment: Text.AlignHCenter
        visible: taskList.count === 0
    }

    // 全屏消息详情（覆盖底部导航栏）
    Item {
        anchors.fill: parent
        visible: detailOpen
        z: 10

        Rectangle {
            anchors.fill: parent
            color: "#FFFFFF"
        }

        Item {
            id: detailHeader
            height: 48
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            z: 2

            Rectangle {
                anchors.fill: parent
                color: "#FFFFFF"
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

                TapHandler {
                    onTapped: detailOpen = false
                }
            }

            Text {
                text: "消息详情"
                font.pixelSize: 18
                color: "#333333"
                anchors.centerIn: parent
            }
        }

        ScrollView {
            anchors.top: detailHeader.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            clip: true
            padding: 16

            Column {
                width: parent.width
                spacing: 16

                Text {
                    text: detailSender !== "" ? ("发送人：" + detailSender) : ""
                    font.pixelSize: 14
                    color: "#666666"
                    visible: detailSender !== ""
                }

                Text {
                    text: detailTime !== "" ? ("时间：" + detailTime) : ""
                    font.pixelSize: 14
                    color: "#666666"
                    visible: detailTime !== ""
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#F0F0F0"
                }

                Text {
                    text: detailText
                    font.pixelSize: 16
                    color: "#333333"
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }
        }
    }
}