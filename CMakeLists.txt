cmake_minimum_required(VERSION 3.10)

project(TLS_PROJ)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(CLIENT_SRC	src/client/client.c)
add_executable(client ${CLIENT_SRC})
set(SERVER_SRC	src/server/server.c)
add_executable(server ${SERVER_SRC})
set(PROXY_SRC src/proxy/proxy.c)
add_executable(proxy ${PROXY_SRC})
find_package(LibreSSL REQUIRED)
target_link_libraries(client tls)
target_link_libraries(server tls)
target_link_libraries(proxy tls)

add_subdirectory(src/lib)
target_link_libraries(client usrlib)
target_link_libraries(server usrlib)
target_link_libraries(proxy usrlib)