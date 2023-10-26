#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_executable(libp2p_echo_server
    libp2p_echo_server.cpp
    )
target_link_libraries(libp2p_echo_server
    Boost::Boost.DI
    p2p_basic_host
    p2p_default_network
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_protocol_echo
    p2p_literals
    )

add_executable(libp2p_echo_client
    libp2p_echo_client.cpp
    )

target_link_libraries(libp2p_echo_client
    Boost::Boost.DI
    p2p_basic_host
    p2p_default_network
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_protocol_echo
    p2p_literals
    )
