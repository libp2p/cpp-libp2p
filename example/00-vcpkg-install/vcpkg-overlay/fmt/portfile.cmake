vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO fmtlib/fmt
        REF 10.1.1
        SHA512 288c349baac5f96f527d5b1bed0fa5f031aa509b4526560c684281388e91909a280c3262a2474d963b5d1bf7064b1c9930c6677fe54a0d8f86982d063296a54c
)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DFMT_DOC=OFF
        -DFMT_TEST=OFF
        -DFMT_INSTALL=ON
        -DCMAKE_CXX_STANDARD=20
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(PACKAGE_NAME fmt CONFIG_PATH lib/cmake/fmt)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")