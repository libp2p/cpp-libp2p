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
    URL  "https://github.com/soramitsu/soralog/archive/v0.0.4.tar.gz"
    SHA1 "df0cd03eddfff42d10445e4746f4927fecfd8663"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )
