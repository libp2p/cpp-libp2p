function(disable_clang_tidy target)
  set_target_properties(${target} PROPERTIES
      C_CLANG_TIDY ""
      CXX_CLANG_TIDY ""
      )
endfunction()

function(addtest test_name)
  add_executable(${test_name} ${ARGN})
  addtest_part(${test_name} ${ARGN})
  target_link_libraries(${test_name}
      GTest::main
      GMock::main
      )
  add_test(
      NAME ${test_name}
      COMMAND $<TARGET_FILE:${test_name}>
  )
  set_target_properties(${test_name} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_bin
      ARCHIVE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test_lib
      LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test_lib
      )
  disable_clang_tidy(${test_name})
endfunction()

function(addtest_part test_name)
  if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
  endif ()
  target_sources(${test_name} PUBLIC
      ${ARGN}
      )
  target_link_libraries(${test_name}
      GTest::gtest
      )
endfunction()

# conditionally applies flag. If flag is supported by current compiler, it will be added to compile options.
function(add_flag flag)
  check_cxx_compiler_flag(${flag} FLAG_${flag})
  if (FLAG_${flag} EQUAL 1)
    add_compile_options(${flag})
  endif ()
endfunction()

function(compile_proto_to_cpp PB_H PB_CC PROTO)
  get_target_property(Protobuf_INCLUDE_DIR protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(Protobuf_PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION_RELEASE)

  if (NOT Protobuf_PROTOC_EXECUTABLE)
    message(FATAL_ERROR "Protobuf_PROTOC_EXECUTABLE is empty")
  endif ()
  if (NOT Protobuf_INCLUDE_DIR)
    message(FATAL_ERROR "Protobuf_INCLUDE_DIR is empty")
  endif ()

  get_filename_component(PROTO_ABS "${PROTO}" REALPATH)
  # get relative (to CMAKE_BINARY_DIR) path of current proto file
  file(RELATIVE_PATH SCHEMA_REL "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
  set(SCHEMA_OUT_DIR ${CMAKE_BINARY_DIR}/generated/${SCHEMA_REL})
  file(MAKE_DIRECTORY ${SCHEMA_OUT_DIR})

  string(REGEX REPLACE "\\.proto$" ".pb.h" GEN_PB_HEADER ${PROTO})
  string(REGEX REPLACE "\\.proto$" ".pb.cc" GEN_PB ${PROTO})

  set(GEN_COMMAND ${Protobuf_PROTOC_EXECUTABLE})
  set(GEN_ARGS ${Protobuf_INCLUDE_DIR})

  add_custom_command(
      OUTPUT ${SCHEMA_OUT_DIR}/${GEN_PB_HEADER} ${SCHEMA_OUT_DIR}/${GEN_PB}
      COMMAND ${GEN_COMMAND}
      ARGS -I${GEN_ARGS} -I${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${SCHEMA_OUT_DIR} ${PROTO_ABS}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS protobuf::protoc
      VERBATIM
  )

  set(${PB_H} ${SCHEMA_OUT_DIR}/${GEN_PB_HEADER} PARENT_SCOPE)
  set(${PB_CC} ${SCHEMA_OUT_DIR}/${GEN_PB} PARENT_SCOPE)
endfunction()

function(add_proto_library NAME)
  set(SOURCES "")
  foreach (PROTO IN ITEMS ${ARGN})
    compile_proto_to_cpp(H C ${PROTO})
    list(APPEND SOURCES ${H} ${C})
  endforeach ()

  add_library(${NAME}
      ${SOURCES}
      )
  target_link_libraries(${NAME}
      protobuf::libprotobuf
      )
  target_include_directories(${NAME} PUBLIC
      ${CMAKE_BINARY_DIR}/generated/core
      )
  disable_clang_tidy(${NAME})
endfunction()
