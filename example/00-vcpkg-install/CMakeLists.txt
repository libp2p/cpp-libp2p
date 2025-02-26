cmake_minimum_required(VERSION 3.15)
project(libp2p-example CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libsecp256k1 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

find_package(libp2p CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE p2p::p2p)