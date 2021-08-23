# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(async)
add_subdirectory(libp2p)

add_library(p2p_testutil INTERFACE)
target_link_libraries(p2p_testutil INTERFACE
    p2p_testutil_peer
    p2p_clock
    )
