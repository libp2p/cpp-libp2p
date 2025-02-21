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
    URL  https://github.com/xDimon/soralog/archive/35634db3126d7dc1784c883713c8e31ebb07d051.tar.gz
    SHA1 ee64d0982cffe0a3856c4e786161518e55a6ffd4
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
    scale
    URL  https://github.com/qdrvm/scale-codec-cpp/archive/24fbd767c8975647a733b45f9c3df92380110fb2.tar.gz
    SHA1 c8570702805cdbacd31bd5caacead82c746ba901
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    qtils
    URL  https://github.com/qdrvm/qtils/archive/9a64dfd6ed0226dec29805aa89d4c713a6f81d9f.tar.gz
    SHA1 16c0269175018e88c33090f9fbdaa230c8fdb911
    CMAKE_ARGS
        FORMAT_ERROR_WITH_FULLTYPE=ON
    KEEP_PACKAGE_SOURCES
)
