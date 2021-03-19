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
    URL  https://github.com/xDimon/soralog/archive/b5a61025530a886abd6869f97d42351e98618c9e.tar.gz
    SHA1 f6e1e60ec386d92adde8814815a62f11907a8414
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )
