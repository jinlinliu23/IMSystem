import QtQuick
import QtQuick.Controls
import client
import "navigation"
import "style"


ApplicationWindow {
    id: appWindow
    visible: true
    width: 400
    height: 700
    title: "ChatClient"

    property Nav nav: Nav {}
    property Theme theme: Theme {}

    /** 全局提示框（挂在窗口上，避免页面内 Dialog 叠两层要按多次确定） */
    function showAlert(message) {
        alertDialog.text = message
        if (alertDialog.opened) {
            alertDialog.close()
            Qt.callLater(function() { alertDialog.open() })
        } else {
            alertDialog.open()
        }
    }

    Dialog {
        id: alertDialog
        anchors.centerIn: parent
        modal: true
        focus: true
        title: "提示"
        standardButtons: Dialog.Ok
        property alias text: alertLabel.text

        Label {
            id: alertLabel
            width: Math.min(appWindow.width * 0.75, 320)
            wrapMode: Label.WordWrap
        }
    }

    // 全局背景绑定主题
    background: Rectangle {
        color: theme.bg
    }

    // 全局字体颜色
    palette {
        text: theme.text
        windowText: theme.text
        buttonText: theme.text
    }

    // 根容器：StackView
    StackView {
        id: stackView
        anchors.fill: parent

        Component.onCompleted: {
            nav.stackView = stackView
            if (typeof ClientFacade !== "undefined" && ClientFacade.isLoggedIn) {
                nav.replace("MainPage")
            } else {
                nav.replace("LoginPage")
            }
        }
    }
}