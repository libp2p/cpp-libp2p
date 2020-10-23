file(GLOB_RECURSE
  ALL_CXX_SOURCE_FILES
  core/*.[ch]pp
  core/*.[ch]
  test/*.[ch]pp
  test/*.[ch]
  )

# Adding clang-format target if executable is found
if(NOT CLANG_FORMAT_BIN)
  find_program(CLANG_FORMAT_BIN NAMES clang-format clang-format-9 clang-format-8 clang-format-7)
endif()

if(CLANG_FORMAT_BIN AND NOT TARGET clang-format)
  message(STATUS "Target clang-format enabled")
  add_custom_target(
    clang-format
    COMMAND "${CLANG_FORMAT_BIN}"
    -i
    ${ALL_CXX_SOURCE_FILES}
  )
endif()
