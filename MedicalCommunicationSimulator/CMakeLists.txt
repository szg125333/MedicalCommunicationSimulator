cmake_minimum_required(VERSION 3.16)
project(MedicalCommunicationSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件列表
set(SOURCES
    src/mian.cpp
    src/factory/MessageBuilderFactory.cpp
    src/messages/FileMeta.cpp
    src/messages/request/QuerySeriesRequest.cpp
    src/messages/request/ImmutableFirstTypeRequest.cpp
    src/messages/response/ImmutableFirstTypeResponse.cpp
    src/Worker/NewNotifyWorker.cpp
    src/Worker/NewRequestWorker.cpp
)

# 头文件目录
include_directories(
    src/core
    src/factory
    src/messages
    src/messages/request
    src/messages/response
    src/Worker
)

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

include_directories(${ZMQ_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS})
link_directories(${ZMQ_LIBRARY_DIRS} ${JSONCPP_LIBRARY_DIRS})

# 可执行文件
add_executable(MedicalCommunicationSimulator ${SOURCES})

# 链接库
target_link_libraries(MedicalCommunicationSimulator
    ${ZMQ_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)
