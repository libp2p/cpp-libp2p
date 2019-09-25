# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_LEAK_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_LEAK_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

set(FLAGS
    -fsanitize=leak
    -g
    )
foreach(FLAG IN LISTS FLAGS)
  add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
  add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

set(ENV{LSAN_OPTIONS} detect_leaks=1)

list(APPEND HUNTER_TOOLCHAIN_UNDETECTABLE_ID "sanitize-leak")
