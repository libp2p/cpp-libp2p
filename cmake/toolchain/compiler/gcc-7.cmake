if(DEFINED POLLY_COMPILER_GCC_7_CMAKE_)
  return()
else()
  set(POLLY_COMPILER_GCC_7_CMAKE_ 1)
endif()

find_program(CMAKE_C_COMPILER gcc-7)
find_program(CMAKE_CXX_COMPILER g++-7)

if(NOT CMAKE_C_COMPILER)
  fatal_error("gcc-7 not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
  fatal_error("g++-7 not found")
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
