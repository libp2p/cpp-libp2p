#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_websocket
    ws_adapter.cpp
    http_to_ws_upgrader.cpp
    http_read_writer.cpp
    )
target_link_libraries(p2p_websocket
    p2p_websocket_connection
    Boost::boost
    )

libp2p_add_library(p2p_websocket_connection
    ws_connection.cpp
    ws_frame.cpp
    ws_reading_state.cpp
    ws_read_writer.cpp
    )
target_link_libraries(p2p_websocket_connection
    Boost::boost
    p2p_byteutil
    p2p_read_buffer
    p2p_write_queue
    p2p_connection_error
    )
