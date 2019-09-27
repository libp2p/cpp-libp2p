# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(testutil_peer
    peer.cpp
    )
target_link_libraries(testutil_peer
    libp2p_peer_id
    )

add_library(testutil_read_writer_helper
    message_read_writer_helper.cpp
    message_read_writer_helper.hpp
    )
target_link_libraries(testutil_read_writer_helper
    uvarint
    )
