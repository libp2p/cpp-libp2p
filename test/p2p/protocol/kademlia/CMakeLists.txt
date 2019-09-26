# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(libp2p_protocol_kademlia_test
    libp2p_protocol_kademlia_test.cpp
    )
target_link_libraries(libp2p_protocol_kademlia_test
    libp2p_protocol_kademlia
    multiaddress
    )


addtest(node_id
    node_id_test.cpp
    )
target_link_libraries(node_id
    sha
    testutil_peer
    )

addtest(routing_table_test
    routing_table_test.cpp
    )
target_link_libraries(routing_table_test
    libp2p_kad_routing_table
    testutil_peer
    )
