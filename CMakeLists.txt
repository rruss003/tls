cmake_minimum_required(VERSION 3.10)

project(TLS_PROJ)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(CLIENT_SRC	src/client/client.c)
add_executable(client ${CLIENT_SRC})
set(SERVER_SRC	src/server/server.c src/server/strlcpy.c)
add_executable(server ${SERVER_SRC})
find_package(LibreSSL REQUIRED)
add_subdirectory(extern/libressl)
target_link_libraries(client tls)
