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
    URL  https://github.com/soramitsu/soralog/archive/bbf24c5f53ef19ec8cbf86d2f334c2724148b11e.tar.gz
    SHA1 caa230cb62c7946840f8b1c1c4675f226ed483fb
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )
