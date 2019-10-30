#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

#######################################
# load and set up hat-trie dependency #
#######################################

include(FeatureSummary)
include(ExternalProject)

libp2p_add_library(hat_trie INTERFACE)

set(GIT_URL https://github.com/Tessil/hat-trie.git)
set(HAT_TRIE_VERSION master)
set(GIT_TAG "v0.6.0")

set_package_properties(hat-trie-imp
    PROPERTIES
    URL ${GIT_URL}
    DESCRIPTION "fast and memory efficient HAT-trie"
    )

externalproject_add(hat-trie-imp
    GIT_REPOSITORY  ${GIT_URL}
    GIT_TAG         ${GIT_TAG}
    GIT_SHALLOW     1
    TLS_VERIFY true
    INSTALL_COMMAND "" # remove install step
    )

add_dependencies(hat_trie hat-trie-imp)

externalproject_get_property(hat-trie-imp source_dir)
externalproject_get_property(hat-trie-imp install_dir)
message("tsl source dir = ${source_dir}")
message("tsl install dir = ${install_dir}")
set(hat-trie_INCLUDE_DIR ${source_dir}/include)
file(MAKE_DIRECTORY ${hat-trie_INCLUDE_DIR})

target_include_directories(hat_trie INTERFACE $<BUILD_INTERFACE:${hat-trie_INCLUDE_DIR}>)
find_library(ht NAMES hat_trie)

#include(CMakePackageConfigHelpers)
#write_basic_package_version_file(
#    "${PROJECT_BINARY_DIR}/hat_trieConfigVersion.cmake"
#    VERSION 1.0
#    COMPATIBILITY AnyNewerVersion
#)

#install(
#    EXPORT hat_trieConfig
#    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libp2p
#    NAMESPACE p2p::
#)

#install(TARGETS mylib
#    EXPORT mylibTargets
#    LIBRARY DESTINATION lib COMPONENT Runtime
#    ARCHIVE DESTINATION lib COMPONENT Development
#    RUNTIME DESTINATION bin COMPONENT Runtime
#    PUBLIC_HEADER DESTINATION include COMPONENT Development
#    BUNDLE DESTINATION bin COMPONENT Runtime
#    )

#configure_package_config_file(
#    "${PROJECT_SOURCE_DIR}/cmake/hat_trieConfig.cmake.in"
#    "${PROJECT_BINARY_DIR}/hat_trieConfig.cmake"
#    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)

#install(EXPORT hat_trie DESTINATION lib/cmake/libp2p)
#install(FILES "${PROJECT_BINARY_DIR}/hat_trieConfigVersion.cmake"
#    "${PROJECT_BINARY_DIR}/hat_trieConfig.cmake"
#    DESTINATION lib/cmake/hat_trie)
#install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

#libp2p_install(hat_trie)
