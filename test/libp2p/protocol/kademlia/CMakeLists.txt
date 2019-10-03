# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(libp2p_protocol_kademlia_test
    libp2p_protocol_kademlia_test.cpp
    )
target_link_libraries(libp2p_protocol_kademlia_test
    p2p_protocol_kademlia
    p2p_multiaddress
    p2p_literals
    )


addtest(node_id
    node_id_test.cpp
    )
target_link_libraries(node_id
    p2p_sha
    p2p_testutil_peer
    p2p_literals
    )

addtest(routing_table_test
    routing_table_test.cpp
    )
target_link_libraries(routing_table_test
    p2p_kad_routing_table
    p2p_testutil_peer
    p2p_literals
    )
