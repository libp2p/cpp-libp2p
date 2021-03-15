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
    URL  https://github.com/xDimon/soralog/archive/e8ad1548efd6b75ca8e6191a1fb2fdd254f6c719.tar.gz
    SHA1 45a8741d4fa6a89c4ee5a744087af886d52c6a18
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )
