# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_kademlia
    routing_table_impl.cpp
    kademlia_impl.cpp
#    kad_server.cpp
#    kad_message.cpp
    session.cpp
    value_store_impl.cpp
    value_store_backend_default.cpp
#    content_providers_store.cpp
#    content_address.cpp
    peer_finder.cpp
    )
target_link_libraries(p2p_kademlia
    scheduler
    p2p_peer_id
    p2p_cid
    p2p_kademlia_message
    p2p_kademlia_error
    )
