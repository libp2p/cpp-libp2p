# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_kad
    routing_table_impl.cpp
    kad_impl.cpp
    kad_server.cpp
    kad_message.cpp
    kad_protocol_session.cpp
    local_value_store.cpp
    content_providers_store.cpp
    content_address.cpp
    )
target_link_libraries(p2p_kad
    scheduler
    p2p_peer_id
    p2p_cid
    p2p_kad_proto
    )
