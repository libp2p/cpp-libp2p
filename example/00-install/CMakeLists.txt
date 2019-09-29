cmake_minimum_required(VERSION 3.12)

project(00-install C CXX)

find_package(libp2p CONFIG REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main
    p2p::p2p_multiaddress
    )
