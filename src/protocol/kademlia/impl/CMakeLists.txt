# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_kademlia
    peer_routing_table_impl.cpp
    content_routing_table_impl.cpp
    kademlia_impl.cpp
    session.cpp
    storage_impl.cpp
    storage_backend_default.cpp
    validator_default.cpp
    put_value_executor.cpp
    get_value_executor.cpp
    add_provider_executor.cpp
    find_providers_executor.cpp
    find_peer_executor.cpp
    )
target_link_libraries(p2p_kademlia
    p2p_basic_scheduler
    p2p_kademlia_message
    p2p_kademlia_error
    )
