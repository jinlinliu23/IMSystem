import QtQuick

/**
 * 文字头像：昵称缩写 + 按 seed（通常为 account）生成的背景色。
 * isGroup 为 true 时显示群聊样式（与联系人页一致）。
 */
Item {
    id: root

    property string displayName: ""
    property string seed: ""
    property bool isGroup: false
    property int size: 40

    implicitWidth: size
    implicitHeight: size

    readonly property string labelText: {
        if (isGroup)
            return "群"
        var name = (displayName || "").trim()
        if (!name.length)
            name = (seed || "").trim()
        if (!name.length)
            return "?"
        if (/[\u4e00-\u9fff\u3400-\u4dbf\uf900-\ufaff]/.test(name))
            return name.length >= 2 ? name.slice(-2) : name
        return name.charAt(0).toUpperCase()
    }

    readonly property color bgColor: {
        if (isGroup)
            return "#e8f4fc"
        var key = (seed || displayName || "?").trim()
        var palette = ["#5B8FF9", "#5AD8A6", "#5D7092", "#F6BD16", "#E86452", "#6DC8EC", "#9270CA", "#FF9D4D"]
        var h = 0
        for (var i = 0; i < key.length; ++i)
            h = ((h << 5) - h) + key.charCodeAt(i)
        return palette[Math.abs(h) % palette.length]
    }

    readonly property color fgColor: isGroup ? (themePrimary || "#4A90D9") : "#FFFFFF"

    property color themePrimary: "#4A90D9"

    Rectangle {
        anchors.fill: parent
        radius: Math.max(4, root.size * 0.15)
        color: root.bgColor

        Text {
            anchors.centerIn: parent
            text: root.labelText
            font.pixelSize: root.isGroup ? Math.max(12, root.size * 0.35)
                                          : Math.max(11, root.size * 0.32)
            font.bold: !root.isGroup
            color: root.fgColor
        }
    }
}
