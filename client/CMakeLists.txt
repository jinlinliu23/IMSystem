cmake_minimum_required(VERSION 3.16)

project(client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IM_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")
set(CLIENT_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cpp")

find_package(Qt6 REQUIRED COMPONENTS Quick Network Sql)

qt_standard_project_setup(REQUIRES 6.10)

set(CLIENT_SOURCES
    main.cpp
    cpp/clientfacade.h
    cpp/clientfacade.cpp
    cpp/core/clientsettings.h
    cpp/core/clientsettings.cpp
    cpp/network/tcpclient.h
    cpp/network/tcpclient.cpp
    cpp/network/protocolframe.h
    cpp/network/clientmessagerouter.h
    cpp/network/clientmessagerouter.cpp
    cpp/model/currentusermodel.h
    cpp/model/currentusermodel.cpp
    cpp/model/conversationlistmodel.h
    cpp/model/conversationlistmodel.cpp
    cpp/model/grouplistmodel.h
    cpp/model/grouplistmodel.cpp
    cpp/model/contactlistmodel.h
    cpp/model/contactlistmodel.cpp
    cpp/model/friendrequestlistmodel.h
    cpp/model/friendrequestlistmodel.cpp
    cpp/model/messagelistmodel.h
    cpp/model/messagelistmodel.cpp
    cpp/service/authservice.h
    cpp/service/authservice.cpp
    cpp/service/chatservice.h
    cpp/service/chatservice.cpp
    cpp/service/contactservice.h
    cpp/service/contactservice.cpp
    cpp/service/groupservice.h
    cpp/service/groupservice.cpp
    cpp/storage/chatlocalstore.h
    cpp/storage/chatlocalstore.cpp
    cpp/ai/messageclassifier.h
    cpp/ai/messageclassifier.cpp
)

qt_add_executable(appclient ${CLIENT_SOURCES})

qt_add_qml_module(appclient
    URI client
    QML_FILES
        qml/Main.qml
        QML_FILES qml/pages/LoginPage.qml
        QML_FILES qml/pages/RegisterPage.qml
        QML_FILES qml/pages/MainPage.qml
        QML_FILES qml/style/Theme.qml
        QML_FILES qml/navigation/Nav.qml
        QML_FILES qml/pages/MessagesPage.qml
        QML_FILES qml/pages/ContactsPage.qml
        QML_FILES qml/pages/ProfilePage.qml
        QML_FILES qml/pages/ChatPage.qml
        QML_FILES qml/pages/AddFriendPage.qml
        QML_FILES qml/pages/FriendRequestsPage.qml
        QML_FILES qml/pages/CreateGroupPage.qml
        QML_FILES qml/pages/GroupInfoPage.qml
        QML_FILES qml/pages/InviteMemberPage.qml
        QML_FILES qml/components/ImAvatar.qml
)

set_target_properties(appclient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appclient PRIVATE
    ${IM_COMMON_DIR}
    ${CLIENT_CPP_DIR}
)

target_link_libraries(appclient
    PRIVATE Qt6::Quick Qt6::Network Qt6::Sql
)

# Android / 桌面部署 QSQLITE 驱动插件
qt_import_plugins(appclient INCLUDE Qt6::QSQLiteDriverPlugin)

install(TARGETS appclient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
