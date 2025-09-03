## Template for add custom hunter config
# Useful when there is a need for a non-default version or arguments of a dependency,
# or when a project not registered in Hunter should be added.
#
# hunter_config(
#     package-name
#     VERSION 0.0.0-package-version
#     CMAKE_ARGS "CMAKE_VARIABLE=value"
# )
#
# hunter_config(
#     package-name
#     URL https://repo/archive.zip
#     SHA1 1234567890abcdef1234567890abcdef12345678
#     CMAKE_ARGS "CMAKE_VARIABLE=value"
# )

# Fix for Protobuf CMake compatibility issue with modern CMake versions
hunter_config(
    Protobuf
    VERSION 3.19.4-p0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
      protobuf_BUILD_TESTS=OFF
      protobuf_BUILD_SHARED_LIBS=OFF
    KEEP_PACKAGE_SOURCES
)

# Fix for c-ares CMake compatibility issue with modern CMake versions
hunter_config(
    c-ares
    VERSION 1.14.0-p0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

# Fix for yaml-cpp CMake compatibility issue with modern CMake versions
hunter_config(
    yaml-cpp
    VERSION 0.6.2-0f9a586-p1
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

# Fix for Boost.DI CMake compatibility issue with modern CMake versions
hunter_config(
    Boost.DI
    VERSION 1.1.0-p1
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    soralog
    VERSION 0.2.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    qtils
    VERSION 0.1.1
    CMAKE_ARGS
      FORMAT_ERROR_WITH_FULLTYPE=ON
    KEEP_PACKAGE_SOURCES
)
