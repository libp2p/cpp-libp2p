# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(p2p_testutil_peer
    peer.cpp
    )
target_link_libraries(p2p_testutil_peer
    p2p_peer_id
    )

add_library(p2p_testutil_read_writer_helper
    message_read_writer_helper.cpp
    message_read_writer_helper.hpp
    )
target_link_libraries(p2p_testutil_read_writer_helper
    p2p_uvarint
    )
