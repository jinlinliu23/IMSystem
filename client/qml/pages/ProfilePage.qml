import QtQuick
import QtQuick.Controls

Item {
    anchors.fill: parent
    property var theme: appWindow.theme

    Column {
        anchors.centerIn: parent
        spacing: 12
        width: parent.width * 0.8

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: typeof ClientFacade !== "undefined" ? ClientFacade.nickname : ""
            font.pixelSize: 24
            font.bold: true
            color: theme ? theme.text : "#333"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: typeof ClientFacade !== "undefined"
                  ? ("账号：" + ClientFacade.username)
                  : ""
            font.pixelSize: 15
            color: theme ? theme.text : "#555"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: typeof ClientFacade !== "undefined"
                  ? ("ID：" + ClientFacade.userId)
                  : ""
            font.pixelSize: 12
            color: "#888"
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 11
            color: "#aaa"
            text: "昵称后续可支持修改；账号为唯一标识"
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "退出登录"
            width: parent.width
            onClicked: {
                if (typeof ClientFacade !== "undefined") {
                    ClientFacade.logout()
                    appWindow.nav.replace("LoginPage")
                }
            }
        }
    }
}
