import QtQuick
import QtQuick.Controls

QtObject {
    property StackView stackView: null

    property var routes: ({
        "LoginPage": "../pages/LoginPage.qml",
        "RegisterPage": "../pages/RegisterPage.qml",
        "MainPage": "../pages/MainPage.qml",
        "ChatPage": "../pages/ChatPage.qml"
    })

    property var componentCache: ({})

    function getComponent(routeName) {
        var url = routes[routeName]
        if (!url) {
            console.warn("Nav: 未知路由 '" + routeName + "'")
            return null
        }
        if (!componentCache[url]) {
            var comp = Qt.createComponent(url)
            if (comp.status === Component.Error) {
                console.warn("Nav: 加载组件失败", url, comp.errorString())
                return null
            }
            componentCache[url] = comp
        }
        return componentCache[url]
    }

    function push(routeName, props) {
        if (!stackView) return
        var comp = getComponent(routeName)
        if (comp) {
            var page = comp.createObject(stackView, props || {})
            if (page) stackView.push(page)
        }
    }

    function replace(routeName, props) {
        if (!stackView) return
        var comp = getComponent(routeName)
        if (comp) {
            var page = comp.createObject(stackView, props || {})
            if (page) stackView.replace(page)
        }
    }

    function pop() {
        if (stackView) stackView.pop()
    }
}