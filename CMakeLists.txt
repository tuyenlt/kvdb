cmake_minimum_required(VERSION 3.16)
project(kvdb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
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

add_executable(local local/buff_array.cpp)

target_include_directories(local
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)

add_executable(kvdb_benchmark
    test/kvdb_benchmark.cpp
    src/net/protocol.cpp
)
target_include_directories(kvdb_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kvdb_benchmark PRIVATE Threads::Threads)