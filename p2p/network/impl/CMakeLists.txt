#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libp2p_router
    router_impl.hpp
    router_impl.cpp
    )
target_link_libraries(libp2p_router
    outcome
    )


add_library(libp2p_listener_manager
    listener_manager_impl.cpp
    )
target_link_libraries(libp2p_listener_manager
    Boost::boost
    multiaddress
    libp2p_peer_id
    )


add_library(libp2p_dialer
    dialer_impl.cpp
    )
target_link_libraries(libp2p_dialer
    Boost::boost
    multiaddress
    libp2p_peer_id
    )


add_library(libp2p_network
    network_impl.cpp
    )
target_link_libraries(libp2p_network
    outcome
    )

add_library(libp2p_transport_manager
    transport_manager_impl.cpp
    transport_manager_impl.hpp
    )
target_link_libraries(libp2p_transport_manager
    multiaddress
    )

add_library(libp2p_connection_manager
    connection_manager_impl.cpp
    )
target_link_libraries(libp2p_connection_manager
    outcome
    )
