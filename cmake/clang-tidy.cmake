if(NOT CLANG_TIDY_BIN)
  find_program(CLANG_TIDY_BIN clang-tidy)
endif()

if(NOT CLANG_TIDY_BIN)
  message(FATAL_ERROR "clang-tidy is not installed. Aborting...")
else()
  message(STATUS "clang-tidy has been found: ${CLANG_TIDY_BIN}")
endif()

set(CMAKE_C_CLANG_TIDY   ${CLANG_TIDY_BIN})
set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_BIN})
