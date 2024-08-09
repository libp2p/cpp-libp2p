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

hunter_config(lsquic
  URL  https://github.com/qdrvm/lsquic/archive/b79ff5c4be9936089d32dc1bc2dac70d54651e80.zip
  SHA1 d628f8d7ec68ec33d7f404fbb7b433c62980aa13
  KEEP_PACKAGE_SOURCES
)
