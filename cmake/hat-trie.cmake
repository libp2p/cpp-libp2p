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
    GIT_REPOSITORY ${GIT_URL}
    GIT_TAG ${GIT_TAG}
    GIT_SHALLOW 1
    TLS_VERIFY true
    INSTALL_COMMAND "" # remove install step
    BUILD_COMMAND "" # remove build step
    TEST_COMMAND "" # remove test step
    )

add_dependencies(hat_trie hat-trie-imp)

externalproject_get_property(hat-trie-imp source_dir)
set(hat-trie_INCLUDE_DIR ${source_dir}/include)
file(MAKE_DIRECTORY ${hat-trie_INCLUDE_DIR})

target_include_directories(hat_trie INTERFACE $<BUILD_INTERFACE:${hat-trie_INCLUDE_DIR}>)
