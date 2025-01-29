add_executable(publisher publisher.cpp)
target_link_libraries(publisher PRIVATE mq::mq)

add_executable(subscriber subscriber.cpp)
target_link_libraries(subscriber PRIVATE mq::mq)
