add_executable(client client.cpp)
target_link_libraries(client mq::mq)

add_executable(server server.cpp)
target_link_libraries(server mq::mq)
