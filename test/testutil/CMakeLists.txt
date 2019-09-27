# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(libp2p)

add_library(testutil INTERFACE)
target_link_libraries(testutil INTERFACE
    testutil_peer
    )
