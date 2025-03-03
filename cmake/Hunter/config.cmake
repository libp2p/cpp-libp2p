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
    URL  https://github.com/xDimon/soralog/archive/4dfffd3d949b1c16a04db2e5756555a4031732f7.tar.gz
    SHA1 60e3dcaab2d8e43f0ed4fd22087677663c618716
    #    VERSION 0.2.4
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    ZLIB
    VERSION 1.3.0-p0
    URL  https://github.com/cpp-pm/zlib/archive/refs/tags/v1.3.0-p0.tar.gz
    SHA1 311ca59e20cbbfe9d9e05196c12c6ae109093987
)

hunter_config(
    qtils
    URL  https://github.com/qdrvm/qtils/archive/1e492cf09a3640570cae59a951502614320c0797.tar.gz
    SHA1 033dd907e2566c95ce2ccf1fa6dd9766bc896894
    CMAKE_ARGS
      FORMAT_ERROR_WITH_FULLTYPE=ON
    KEEP_PACKAGE_SOURCES
)
