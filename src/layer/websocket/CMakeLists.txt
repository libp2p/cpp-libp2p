#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_websocket
    ws_adaptor.cpp
    wss_adaptor.cpp
    )
target_link_libraries(p2p_websocket
    p2p_websocket_connection
    Boost::boost
    )

libp2p_add_library(p2p_websocket_connection
    ssl_connection.cpp
    ws_connection.cpp
    )
target_link_libraries(p2p_websocket_connection
    Boost::boost
    p2p_byteutil
    p2p_read_buffer
    p2p_write_queue
    p2p_connection_error
    p2p_sha
    )
