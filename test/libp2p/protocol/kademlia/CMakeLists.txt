# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

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
    p2p_kad
    p2p_testutil_peer
    p2p_literals
    )
