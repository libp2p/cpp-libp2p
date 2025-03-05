vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO libp2p/cpp-libp2p
  REF 5d0013db9850a1c23dfe9ef6673077959b2d784f
  SHA512 fdc97ef948edccf9f31c0b7807a813a79975c822e96c8dcb48f14a3827505422b47f1f693e62e0bfe56689079992ee548f5ca631bc84d6e4098eac220b5cf98d
)
vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DCMAKE_CXX_STANDARD=20
        -DCMAKE_CXX_STANDARD_REQUIRED=ON
        -DTESTING=OFF
        -DEXAMPLES=OFF
)
vcpkg_cmake_install()
#vcpkg_cmake_config_fixup(PACKAGE_NAME "libsecp256k1")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
