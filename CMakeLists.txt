set(CLIENT_SRC client.c)
add_executable(client ${CLIENT_SRC})
#target_link_libraries(client LibreSSL::TLS)

set(SERVER_SRC server.c strlcpy.c)
add_executable(server ${SERVER_SRC})
#target_link_libraries(server LibreSSL::TLS)