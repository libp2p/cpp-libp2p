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
    VERSION 0.2.5
    URL  https://github.com/qdrvm/soralog/archive/refs/tags/v0.2.5.tar.gz
    SHA1 1dafdb9e1921b4069f9e1dad0d0acfae24166bd2
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    qtils
    VERSION 0.1.0
    URL  https://github.com/qdrvm/qtils/archive/refs/tags/v0.1.0.tar.gz
    SHA1 acc28902af7dc5d74ac33d486ad2261906716f5e
    CMAKE_ARGS
      FORMAT_ERROR_WITH_FULLTYPE=ON
    KEEP_PACKAGE_SOURCES
)
