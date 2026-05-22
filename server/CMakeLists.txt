cmake_minimum_required(VERSION 3.16)

project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IM_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")

add_executable(server main.cpp
    network/eventloop.h network/eventloop.cpp
    network/channel.h network/channel.cpp
    network/socket.h network/socket.cpp
    network/tcpserver.h network/tcpserver.cpp
    network/tcpconnection.h network/tcpconnection.cpp
    network/acceptor.h network/acceptor.cpp
    network/msgnode.h network/msgnode.cpp

    utility/singleton.h
    utility/logicsystem.h utility/logicsystem.cpp
    utility/logicnode.h utility/logicnode.cpp
    network/networkconst.h
    utility/msgIdconst.h
    utility/mesrout.h utility/mesrout.cpp
    utility/passwordhash.h utility/passwordhash.cpp
    utility/onlineuserproxy.h
    utility/onlineusersmanager.h utility/onlineusersmanager.cpp
    utility/offlinemessagekind.h
    utility/messagedispatcher.h utility/messagedispatcher.cpp
    business/handler.h business/handler.cpp
    business/registerhandler.h business/registerhandler.cpp
    business/loginhandler.h business/loginhandler.cpp
    business/searchuserhandler.h business/searchuserhandler.cpp
    business/friendhandler.h business/friendhandler.cpp
    business/privatechathandler.h business/privatechathandler.cpp
    database/databasemanager.h database/databasemanager.cpp
)

target_include_directories(server PRIVATE ${IM_COMMON_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)

target_link_libraries(server PRIVATE
    PkgConfig::JSONCPP
    PkgConfig::SQLITE3
    PkgConfig::OPENSSL
)

include(GNUInstallDirs)
install(TARGETS server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
