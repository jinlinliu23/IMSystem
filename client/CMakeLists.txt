cmake_minimum_required(VERSION 3.16)

project(client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IM_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appclient
    main.cpp
    cpp/tcpclient.h cpp/tcpclient.cpp
)


qt_add_qml_module(appclient
    URI client
    QML_FILES
        qml/Main.qml
        QML_FILES qml/pages/LoginPage.qml
        QML_FILES qml/pages/RegisterPage.qml
        QML_FILES qml/pages/MainPage.qml

        QML_FILES qml/style/Theme.qml
        QML_FILES qml/navigation/Nav.qml
        SOURCES cpp/clientfacade.h cpp/clientfacade.cpp
        QML_FILES qml/pages/MessagesPage.qml
        QML_FILES qml/pages/ContactsPage.qml
        QML_FILES qml/pages/ProfilePage.qml
        QML_FILES qml/pages/ChatPage.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appclient PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appclient
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appclient PRIVATE ${IM_COMMON_DIR})

target_link_libraries(appclient
    PRIVATE Qt6::Quick Qt6::Network
)

install(TARGETS appclient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
