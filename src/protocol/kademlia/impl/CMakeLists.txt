# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_protocol_kademlia
    kad_impl.cpp
    )
target_link_libraries(p2p_protocol_kademlia
    p2p_peer_id
    Boost::boost
    )


libp2p_add_library(p2p_kad_routing_table
    routing_table_impl.cpp
    )
target_link_libraries(p2p_kad_routing_table
    Boost::boost
    p2p_peer_id
    )
