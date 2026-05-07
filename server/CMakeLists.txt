cmake_minimum_required(VERSION 3.16)

project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server main.cpp
    network/TCPlisteningsocket.h network/TCPlisteningsocket.cpp
    network/eventloop.h network/eventloop.cpp
    network/channel.h network/channel.cpp
    network/socket.h network/socket.cpp
    network/tcpserver.h network/tcpserver.cpp
    network/tcpconnection.h network/tcpconnection.cpp
    network/acceptor.h network/acceptor.cpp)

include(GNUInstallDirs)
install(TARGETS server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
