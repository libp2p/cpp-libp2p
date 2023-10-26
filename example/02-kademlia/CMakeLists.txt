#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_executable(rendezvous_chat
    rendezvous_chat.cpp
    )

target_link_libraries(rendezvous_chat
    p2p_basic_host
    p2p_default_network
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_literals
    p2p_kademlia
    p2p_multiaddress
    )
