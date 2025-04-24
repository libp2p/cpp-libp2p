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
    URL https://github.com/qdrvm/soralog/archive/cf70b08f8ef86696861272bc7195242b718d23cd.tar.gz
    SHA1 358acbcdd9696c4b91141ee77606e097ddaa1b9e
)

hunter_config(
    qtils
    URL https://github.com/qdrvm/qtils/archive/77479adc6fa4eeceb81dbf78ecab7766fdb7c898.tar.gz
    SHA1 016750cce6821904ff7d8e026828fca5a83ea46a
    CMAKE_ARGS
      FORMAT_ERROR_WITH_FULLTYPE=ON
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    fmt
    URL  https://github.com/fmtlib/fmt/archive/refs/tags/11.1.4.tar.gz
    SHA1 045b14fcdc6356eeb2d83feeb1c79e58215517f8
    CMAKE_ARGS
    CMAKE_POSITION_INDEPENDENT_CODE=TRUE
)
