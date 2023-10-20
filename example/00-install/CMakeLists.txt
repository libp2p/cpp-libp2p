cmake_minimum_required(VERSION 3.12)

# reuse HunterGate...
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/Hunter/HunterGate.cmake)
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.216.tar.gz"
    SHA1 "a492a1879bee60c29f7af17ef56b45f0d7766241"
)


project(00-install C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

hunter_add_package(Boost)
find_package(Boost CONFIG REQUIRED)

# look for libp2p in OS
find_package(libp2p CONFIG REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main
    p2p::p2p_multiaddress
    )
