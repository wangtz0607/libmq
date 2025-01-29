add_executable(requester requester.cpp)
target_link_libraries(requester PRIVATE mq::mq)

add_executable(replier replier.cpp)
target_link_libraries(replier PRIVATE mq::mq)
