#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_yamux
    yamux.cpp
    )
target_link_libraries(p2p_yamux
    p2p_yamuxed_connection
    )

libp2p_add_library(p2p_yamuxed_connection
    yamuxed_connection.cpp
    yamux_frame.cpp
    yamux_stream.cpp
    yamux_reading_state.cpp
    )
target_link_libraries(p2p_yamuxed_connection
    Boost::boost
    p2p_byteutil
    p2p_peer_id
    p2p_read_buffer
    p2p_write_queue
    p2p_connection_error
    )
