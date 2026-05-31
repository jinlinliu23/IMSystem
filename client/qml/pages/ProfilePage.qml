import QtQuick
import QtQuick.Controls

Item {
    anchors.fill: parent
    property var t: appWindow.theme

    readonly property string currentNickname:
        (typeof ClientFacade !== "undefined" && ClientFacade.currentUser)
        ? ClientFacade.currentUser.nickname : ""
    readonly property string currentAccount:
        (typeof ClientFacade !== "undefined" && ClientFacade.currentUser)
        ? ClientFacade.currentUser.account : ""

    Rectangle {
        anchors.fill: parent
        color: "#EDEDED"
    }

    Column {
        anchors.fill: parent
        anchors.topMargin: 12
        spacing: 12

        Rectangle {
            width: parent.width
            height: 84
            color: "#FFFFFF"

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                spacing: 14

                ImAvatar {
                    size: 56
                    displayName: currentNickname
                    seed: currentAccount
                    themePrimary: t.primary
                    anchors.verticalCenter: parent.verticalCenter
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    Text {
                        text: currentNickname
                        font.pixelSize: 18
                        color: "#1a1a1a"
                    }

                    Text {
                        text: "账号：" + currentAccount
                        font.pixelSize: 13
                        color: "#999"
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 44
            color: "#FFFFFF"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 0.5
                color: "#E5E5E5"
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                text: "退出登录"
                font.pixelSize: 16
                color: "#1a1a1a"
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 16
                text: ">"
                font.pixelSize: 16
                color: "#C7C7CC"
            }

            Rectangle {
                anchors.fill: parent
                color: logoutTap.pressed ? "#EDEDED" : "transparent"
            }

            TapHandler {
                id: logoutTap
                onTapped: {
                    if (typeof ClientFacade !== "undefined") {
                        ClientFacade.logout()
                        appWindow.nav.replace("LoginPage")
                    }
                }
            }
        }
    }
}
