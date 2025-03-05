vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO qdrvm/soralog
  REF refs/tags/v0.2.5
  SHA512 47375cc61c78ebc4119781bf19ce3b92c4a5a40ed4dc77c0156ac0750df1e4d13455bf6f60d9ea2f0b7bf7dda75423eed320edce453617ab06d6c1c9a8a8843c
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
#vcpkg_cmake_config_fixup(PACKAGE_NAME "soralog")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
