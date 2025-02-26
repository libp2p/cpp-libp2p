vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO masterjedy/hat-trie
  REF 4fdfc75e75276185eed4b748ea09671601101b8e
  SHA512 1f8e216037d06909a80dc89550a667cb1a8c64270c91b0ea5585c98f318fdbfe863a9766c9fadfb3da581b248fcd6b6b13576a2f855c61b7587516c38947c457
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
#vcpkg_cmake_config_fixup(PACKAGE_NAME "tsl-hat-trie")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
