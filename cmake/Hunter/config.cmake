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

hunter_config(BoringSSL
  URL  https://github.com/qdrvm/boringssl/archive/f8065eabdb6695251467b4236161f5c63fc0be55.zip
  SHA1 c9b7244513569c46d847cadf289633c6e3d52188
  KEEP_PACKAGE_SOURCES
)
