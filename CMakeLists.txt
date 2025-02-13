cmake_minimum_required(VERSION 3.5.0)

project(libmq VERSION 0.1.0 LANGUAGES CXX)

option(LIBMQ_EXAMPLES "Build examples." ON)
option(LIBMQ_NO_EXCEPTIONS "-fno-exceptions" ON)
option(LIBMQ_NO_RTTI "-fno-rtti" ON)

add_library(mq
    src/event/EventLoop.cpp
    src/event/Timer.cpp
    src/event/Watcher.cpp
    src/message/MultiplexingReplier.cpp
    src/message/MultiplexingRequester.cpp
    src/message/Publisher.cpp
    src/message/Replier.cpp
    src/message/Requester.cpp
    src/message/Subscriber.cpp
    src/net/Acceptor.cpp
    src/net/FramingAcceptor.cpp
    src/net/FramingSocket.cpp
    src/net/Ip6Addr.cpp
    src/net/IpAddr.cpp
    src/net/NetworkInterface.cpp
    src/net/ReadLineAcceptor.cpp
    src/net/ReadLineSocket.cpp
    src/net/Socket.cpp
    src/net/Tcp6Endpoint.cpp
    src/net/TcpEndpoint.cpp
    src/net/UnixEndpoint.cpp
    src/rpc/RpcClient.cpp
    src/rpc/RpcServer.cpp
    src/utils/Buffer.cpp
    src/utils/Executor.cpp
    src/utils/Logging.cpp
    src/utils/ThreadPool.cpp
)
target_compile_features(mq PUBLIC cxx_std_23)
target_include_directories(mq PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

if(LIBMQ_NO_EXCEPTIONS)
    target_compile_options(mq PUBLIC -fno-exceptions)
endif()

if(LIBMQ_NO_RTTI)
    target_compile_options(mq PUBLIC -fno-rtti)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(mq PUBLIC -ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=)
endif()

add_library(mq::mq ALIAS mq)

install(TARGETS mq EXPORT mq-targets)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION include)
install(EXPORT mq-targets
        FILE mq-targets.cmake
        NAMESPACE mq::
        DESTINATION lib/cmake/mq)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/mq-config.cmake.in"
    "${PROJECT_BINARY_DIR}/mq-config.cmake"
    INSTALL_DESTINATION lib/cmake/mq
)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/mq-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${PROJECT_BINARY_DIR}/mq-config.cmake"
    "${PROJECT_BINARY_DIR}/mq-config-version.cmake"
    DESTINATION lib/cmake/mq
)

export(EXPORT mq-targets
       FILE "${PROJECT_BINARY_DIR}/mq-targets.cmake"
       NAMESPACE mq::)

if(LIBMQ_EXAMPLES)
    add_subdirectory(examples)
endif()

add_executable(example main.cpp)
target_link_libraries(example PRIVATE mq::mq)
