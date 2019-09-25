# Copyright (c) 2014-2017, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_THREAD_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_THREAD_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize=thread")
add_cache_flag(CMAKE_CXX_FLAGS "-g")

add_cache_flag(CMAKE_C_FLAGS "-fsanitize=thread")
add_cache_flag(CMAKE_C_FLAGS "-g")

add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread")
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread")
