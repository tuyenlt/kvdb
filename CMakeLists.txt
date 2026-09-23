cmake_minimum_required(VERSION 3.16)
project(kvdb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(kvdb
    ${SOURCES}
)


target_include_directories(kvdb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(client client/client.cpp src/net/protocol.cpp)

target_include_directories(client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_protocol test/test.cpp src/net/protocol.cpp)

target_include_directories(test_protocol
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)