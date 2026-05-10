cmake_minimum_required(VERSION 3.16)

project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


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
    )

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)

target_link_libraries(server PRIVATE PkgConfig::JSONCPP)

include(GNUInstallDirs)
install(TARGETS server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
