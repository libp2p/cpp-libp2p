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
    URL  "https://github.com/xDimon/soralog/archive/8fbddeef627eeb0456910ba92e89a5b4d9cd75ed.tar.gz"
    SHA1 "98fa62ef43753ad80b2d64756aaa6fa6a49625c1"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )
