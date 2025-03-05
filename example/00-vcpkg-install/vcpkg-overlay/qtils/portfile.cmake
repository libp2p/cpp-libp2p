vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO qdrvm/qtils
  REF 9a64dfd6ed0226dec29805aa89d4c713a6f81d9f
  SHA512 bab32974f6926054ea11b0e6d8549d2b62e1def75cb4464ca47895bdd32d624ff6b9e25d10d7075581c4310c872b71560dfce76e92a60e83949b4af0276768ec
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "qtils")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
