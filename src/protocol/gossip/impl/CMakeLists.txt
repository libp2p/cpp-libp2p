# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_gossip
    common.cpp
    message_builder.cpp
    message_parser.cpp
    gossip_core.cpp
    local_subscriptions.cpp
    remote_subscriptions.cpp
    topic_subscriptions.cpp
    peer_set.cpp
    peer_context.cpp
    message_cache.cpp
    connectivity.cpp
    stream.cpp
    )
target_link_libraries(p2p_gossip
    Boost::boost
    p2p_byteutil
    p2p_multiaddress
    p2p_varint_reader
    subscription
    p2p_peer_id
    p2p_cid
    p2p_gossip_proto
    )
