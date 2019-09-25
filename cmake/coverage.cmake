set(CMAKE_BUILD_TYPE Debug)

include(cmake/3rdparty/CodeCoverage.cmake)

append_coverage_compiler_flags()


set(COVERAGE_LCOV_EXCLUDES
    '${CMAKE_SOURCE_DIR}/deps/*'
    '${CMAKE_SOURCE_DIR}/build/*'
    '${CMAKE_SOURCE_DIR}/cmake-build-debug/*'
    )
set(COVERAGE_GCOVR_EXCLUDES ${COVERAGE_LCOV_EXCLUDES})

setup_target_for_coverage_gcovr_xml(
    NAME ctest_coverage
    EXECUTABLE ctest
)

setup_target_for_coverage_gcovr_html(
    NAME ctest_coverage_html
    EXECUTABLE ctest
)
