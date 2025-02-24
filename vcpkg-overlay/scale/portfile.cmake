vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO qdrvm/scale-codec-cpp
  REF 34e980bb539af19d5d1c28319f5618f40e9ec503
  SHA512 50c3984d2f7dd90b3918d8a88165ac5b2533b7e16fccd865280f36048d8386f3a0831799905d23e6275c89f1d82135d0655effae7c6c43c78f823e6d359e6685
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "scale")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
