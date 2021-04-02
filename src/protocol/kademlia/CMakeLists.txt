#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_kademlia_message
    message.cpp
    impl/content_id.cpp
    )
target_link_libraries(p2p_kademlia_message
    p2p_cid
    p2p_kademlia_proto
    p2p_multiaddress
    p2p_peer_id
    )

libp2p_add_library(p2p_kademlia_error
    error.cpp
    )
target_link_libraries(p2p_kademlia_error
    Boost::boost
    )

add_subdirectory(protobuf)
add_subdirectory(impl)
