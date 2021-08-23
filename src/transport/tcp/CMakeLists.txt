# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_tcp_connection tcp_connection.cpp)
target_link_libraries(p2p_tcp_connection
    Boost::boost
    p2p_multiaddress
    p2p_upgrader_session
    p2p_logger
    p2p_connection_error
    )

libp2p_add_library(p2p_tcp_listener tcp_listener.cpp)
target_link_libraries(p2p_tcp_listener
    p2p_tcp_connection
    p2p_upgrader_session
    )

libp2p_add_library(p2p_tcp tcp_transport.cpp)
target_link_libraries(p2p_tcp
    p2p_tcp_connection
    p2p_tcp_listener
    )
