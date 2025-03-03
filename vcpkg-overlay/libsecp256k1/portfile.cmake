vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO qdrvm/libsecp256k1
  REF 4370b9c336f86457c0b7bb97cd1ac6a281d951fa
  SHA512 ed4660a66d8d74d5d5a27e54a247fe89d0fab4a5618ace27fe384690eebe296b89ababb7eb8a0184d64c339a27c7882306ecefb3fc8bf8c554ca3e244df627e5
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
#vcpkg_cmake_config_fixup(PACKAGE_NAME "libsecp256k1")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
