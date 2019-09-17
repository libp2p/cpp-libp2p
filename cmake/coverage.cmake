set(CMAKE_BUILD_TYPE Debug)

include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/CodeCoverage.cmake)

append_coverage_compiler_flags()

set(COVERAGE_LCOV_EXCLUDES
    'deps/*'
    'build/*'
    )

setup_target_for_coverage_gcovr_xml(
    NAME ctest_coverage                    # New target name
    EXECUTABLE ctest
)

setup_target_for_coverage_gcovr_html(
    NAME ctest_coverage_html               # New target name
    EXECUTABLE ctest
)
