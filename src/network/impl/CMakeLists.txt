#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_router
    router_impl.cpp
    )
target_link_libraries(p2p_router
    outcome
    )


add_library(p2p_listener_manager
    listener_manager_impl.cpp
    )
target_link_libraries(p2p_listener_manager
    Boost::boost
    p2p_multiaddress
    p2p_peer_id
    )


add_library(p2p_dialer
    dialer_impl.cpp
    )
target_link_libraries(p2p_dialer
    Boost::boost
    p2p_multiaddress
    p2p_peer_id
    )


add_library(p2p_network
    network_impl.cpp
    )
target_link_libraries(p2p_network
    outcome
    )

add_library(p2p_transport_manager
    transport_manager_impl.cpp
    )
target_link_libraries(p2p_transport_manager
    p2p_multiaddress
    )

add_library(p2p_connection_manager
    connection_manager_impl.cpp
    )
target_link_libraries(p2p_connection_manager
    outcome
    )
