vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO qdrvm/qtils
  REF refs/tags/v0.1.0
  SHA512 301987eefc98b66c42dcf731d73c11c3e9835098fc3d9a1b8e3adef9b73dad6b0198019d416e1809956620377b48e575157d56b278dcdcf65a24ecdfc134605e
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "qtils")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
