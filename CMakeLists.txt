cmake_minimum_required(VERSION 3.12)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx17.cmake"
    CACHE
    FILEPATH
    "Default toolchain"
    )


include("cmake/Hunter/init.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.171.tar.gz"
    SHA1 "5d68bcca78eee347239ca5f4d34f4b6c12683154"
    LOCAL
)

project(libp2p C CXX)

include(cmake/print.cmake)
print("C flags: ${CMAKE_C_FLAGS}")
print("CXX flags: ${CMAKE_CXX_FLAGS}")
print("Using CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
link_libraries(Threads::Threads)
if (THREADS_HAVE_PTHREAD_ARG)
  link_libraries(pthread)
endif ()

include(CheckCXXCompilerFlag)
include(cmake/dependencies.cmake)
include(cmake/functions.cmake)
include(cmake/san.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLANG_FORMAT "Enable clang-format target" ON)
option(CLANG_TIDY "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE "Enable generation of coverage info" OFF)
# sanitizers will be enabled only for libp2p, and will be disabled for dependencies
option(ASAN "Enable address sanitizer" OFF)
option(LSAN "Enable leak sanitizer" OFF)
option(MSAN "Enable memory sanitizer" OFF)
option(TSAN "Enable thread sanitizer" OFF)
option(UBSAN "Enable UB sanitizer" OFF)


## setup compilation flags
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(AppleClang|Clang|GNU)$")
  # enable those flags
  add_flag(-Wall)
  add_flag(-Wextra)
  add_flag(-Woverloaded-virtual)     # warn if you overload (not override) a virtual function
  add_flag(-Wformat=2)               # warn on security issues around functions that format output (ie printf)
  add_flag(-Wmisleading-indentation) # (only in GCC >= 6.0) warn if indentation implies blocks where blocks do not exist
  add_flag(-Wduplicated-cond)        # (only in GCC >= 6.0) warn if if / else chain has duplicated conditions
  add_flag(-Wduplicated-branches)    # (only in GCC >= 7.0) warn if if / else branches have duplicated code
  add_flag(-Wnull-dereference)       # (only in GCC >= 6.0) warn if a null dereference is detected
  add_flag(-Wdouble-promotion)       # (GCC >= 4.6, Clang >= 3.8) warn if float is implicit promoted to double
  add_flag(-Wsign-compare)
  add_flag(-Wtype-limits)            # size_t - size_t >= 0 -> always true

  # disable those flags
  add_flag(-Wno-unused-command-line-argument)    # clang: warning: argument unused during compilation: '--coverage' [-Wunused-command-line-argument]
  add_flag(-Wno-unused-parameter)    # prints too many useless warnings
  add_flag(-Wno-format-nonliteral)   # prints way too many warnings from spdlog
  add_flag(-Wno-gnu-zero-variadic-macro-arguments)   # https://stackoverflow.com/questions/21266380/is-the-gnu-zero-variadic-macro-arguments-safe-to-ignore

  # promote to errors
  add_flag(-Werror-unused-lambda-capture)  # error if lambda capture is unused
  add_flag(-Werror-return-type)      # warning: control reaches end of non-void function [-Wreturn-type]
  add_flag(-Werror-non-virtual-dtor) # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
  add_flag(-Werror-sign-compare)     # warn the user if they compare a signed and unsigned numbers
  add_flag(-Werror-reorder)          # field '$1' will be initialized after field '$2'
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  # TODO(warchant): add flags https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md#msvc
endif ()

if (COVERAGE)
  include(cmake/coverage.cmake)
endif ()
if (CLANG_TIDY)
  include(cmake/clang-tidy.cmake)
endif ()
if (CLANG_FORMAT)
  include(cmake/clang-format.cmake)
endif ()

add_subdirectory(deps)

include_directories(
    # project includes
    ${PROJECT_SOURCE_DIR}/include
)

include_directories(
    SYSTEM
    # system includes
    deps/di/include
    deps/di/extension/include
    deps/hat-trie/include
    deps/outcome
    deps/spdlog/include
)

add_subdirectory(src)

include(GNUInstallDirs)

include(cmake/install.cmake)

install_deps_headers()

libp2p_install(
    TARGETS
    libp2p
    libp2p_default_network
    libp2p_network
    libp2p_tcp
    libp2p_yamux
    libp2p_plaintext
    libp2p_connection_manager
    libp2p_transport_manager
    libp2p_listener_manager
    libp2p_identity_manager
    libp2p_dialer
    libp2p_router
    multiselect
    random_generator
    key_generator
    key_validator
    key_marshaller
    libp2p_tcp_connection
    libp2p_tcp_listener
    libp2p_yamuxed_connection
    libp2p_security_error
    plaintext_exchange_message_marshaller
    crypto_error
    multiaddress
    libp2p_peer_id
    uvarint
    multihash
    crypto_key
    libp2p_upgrader
    libp2p_upgrader_session
    converter_utils
    multibase_codec
    conversion_error
    converters
    hexutil
    keys_proto
    plaintext_protobuf
    logger
    sha
)

include(CMakePackageConfigHelpers)

set(CONFIG_INCLUDE_DIRS ${CMAKE_INSTALL_FULL_INCLUDEDIR}/libp2p)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/libp2pConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libp2pConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libp2p
    )

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libp2pConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libp2p
    )

export(PACKAGE libp2p)

enable_testing()
add_subdirectory(test)
