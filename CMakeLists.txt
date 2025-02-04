cmake_minimum_required(VERSION 3.5.0)

project(libmq VERSION 0.1.0 LANGUAGES CXX)

option(LIBMQ_EXAMPLES "Build examples." ON)

add_library(mq
    src/event/EventLoop.cpp
    src/event/Timer.cpp
    src/event/Watcher.cpp
    src/message/MultiplexingReplier.cpp
    src/message/MultiplexingRequester.cpp
    src/message/Publisher.cpp
    src/message/Subscriber.cpp
    src/message/Replier.cpp
    src/message/Requester.cpp
    src/net/Acceptor.cpp
    src/net/FramingAcceptor.cpp
    src/net/FramingSocket.cpp
    src/net/IPV4Addr.cpp
    src/net/IPV6Addr.cpp
    src/net/NetworkInterface.cpp
    src/net/Socket.cpp
    src/net/TCPV4Endpoint.cpp
    src/net/TCPV6Endpoint.cpp
    src/net/UnixEndpoint.cpp
    src/rpc/RPCClient.cpp
    src/rpc/RPCServer.cpp
    src/utils/Buffer.cpp
    src/utils/Executor.cpp
    src/utils/Logging.cpp
    src/utils/ThreadPool.cpp
)
target_compile_features(mq PUBLIC cxx_std_23)
target_compile_options(mq PUBLIC -ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=)
target_include_directories(mq PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)
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
