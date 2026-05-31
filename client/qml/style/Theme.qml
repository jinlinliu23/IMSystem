import QtQuick

QtObject {
    // 从 C++ 读取主题模式（暂默认 false）
    property bool darkMode: (typeof ClientFacade !== "undefined") ? (ClientFacade.themeMode === 1) : false

    property color bg: darkMode ? "#1e1e1e" : "#f5f5f5"
    property color text: darkMode ? "#e0e0e0" : "#1e1e1e"
    property color primary: "#007aff"
    property color surface: darkMode ? "#f5f5f5" : "#F0F0F0"
    property color border: darkMode ? "#f5f5f5" : "#e0e0e0"
}