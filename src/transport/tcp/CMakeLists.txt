# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_tcp_connection tcp_connection.cpp)
target_link_libraries(libp2p_tcp_connection
    Boost::boost
    multiaddress
    libp2p_upgrader_session
    )

add_library(libp2p_tcp_listener tcp_listener.cpp)
target_link_libraries(libp2p_tcp_listener
    libp2p_tcp_connection
    libp2p_upgrader_session
    )

add_library(libp2p_tcp tcp_transport.cpp)
target_link_libraries(libp2p_tcp
    libp2p_tcp_connection
    libp2p_tcp_listener
    )
