cmake_minimum_required(VERSION 3.10.2)
project(tls)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(CLIENT_SRC	src/client/client.c src/rw.c src/hash.c)
add_executable(client ${CLIENT_SRC})
add_subdirectory(extern/libressl)
target_link_libraries(client tls)

set(SERVER_SRC	src/server/server.c src/rw.c)
add_executable(server ${SERVER_SRC})
target_link_libraries(server tls)

set(PROXY_SRC	src/proxy/proxy.c src/rw.c src/bloomfilter.c)
add_executable(proxy ${PROXY_SRC})
target_link_libraries(proxy tls)

find_package(LibreSSL REQUIRED)
