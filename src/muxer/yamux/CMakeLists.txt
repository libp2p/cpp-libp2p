#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_yamux
    yamux.cpp
    )
target_link_libraries(p2p_yamux
    p2p_yamuxed_connection
    p2p_peer_id
    )


libp2p_add_library(p2p_yamuxed_connection
    yamuxed_connection.cpp
    yamux_frame.cpp
    yamux_stream.cpp
    )
target_link_libraries(p2p_yamuxed_connection
    Boost::boost
    p2p_logger
    p2p_byteutil
    p2p_peer_id
    )
