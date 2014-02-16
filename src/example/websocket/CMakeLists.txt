file(COPY wsclient.html DESTINATION ${CMAKE_BINARY_DIR}/out)

add_executable(simplewebsocketserver simplewebsocketserver.cpp)
target_link_libraries(simplewebsocketserver jsonrpc)
