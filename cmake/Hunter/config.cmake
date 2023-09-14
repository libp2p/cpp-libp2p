## Template for add custom hunter config
# Useful when there is a need for a non-default version or arguments of a dependency,
# or when a project not registered in soramistu-hunter should be added.
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

hunter_config(soralog
    URL  https://github.com/qdrvm/soralog/archive/1938cb9a535cf7ec18ad4b4e63ea59f21a7b36e6.tar.gz
    SHA1 d79e3071420d3e291b417db1759b857ff54839ac
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )
