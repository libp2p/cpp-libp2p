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

hunter_config(
    soralog
    URL  https://github.com/qdrvm/soralog/archive/9f21b763ad7911a1398bf5644c6e5be890cbdba1.zip
    SHA1 8dd612fa9370db704d9951d808e1d5ddce1ddfdd
)
