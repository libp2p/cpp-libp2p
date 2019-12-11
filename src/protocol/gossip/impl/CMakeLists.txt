# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_gossip
    common.cpp
    gossip_wire_protocol.cpp
    peers.cpp
    )
target_link_libraries(p2p_gossip
    Boost::boost
    p2p_peer_id
    p2p_cid
    p2p_gossip_proto
    p2p_logger
    )
