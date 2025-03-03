vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO xDimon/soralog
  REF 4dfffd3d949b1c16a04db2e5756555a4031732f7
  SHA512 4a8f6066433e6bde504454ea256915e8e6975c060eabdaebd031df409d348b8c22ccd716ec11c4466b0f4322b1f2524350dcc77c3615a8511cba9886f88e7260
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
#vcpkg_cmake_config_fixup(PACKAGE_NAME "soralog")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
