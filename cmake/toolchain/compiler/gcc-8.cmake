if(DEFINED POLLY_COMPILER_GCC_8_CMAKE_)
  return()
else()
  set(POLLY_COMPILER_GCC_8_CMAKE_ 1)
endif()

find_program(CMAKE_C_COMPILER gcc-8)
find_program(CMAKE_CXX_COMPILER g++-8)

if(NOT CMAKE_C_COMPILER)
  fatal_error("gcc-8 not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
  fatal_error("g++-8 not found")
endif()

set(
    CMAKE_C_COMPILER
    "${CMAKE_C_COMPILER}"
    CACHE
    STRING
    "C compiler"
    FORCE
)

set(
    CMAKE_CXX_COMPILER
    "${CMAKE_CXX_COMPILER}"
    CACHE
    STRING
    "C++ compiler"
    FORCE
)
