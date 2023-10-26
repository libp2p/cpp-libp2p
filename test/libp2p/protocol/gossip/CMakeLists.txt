#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(gossip_structures_test
    gossip_structures_test.cpp
    )
target_link_libraries(gossip_structures_test
    p2p_gossip
    p2p_testutil_peer
    p2p_basic_scheduler
    )

addtest(gossip_local_subs_test
    gossip_local_subs_test.cpp
    )
target_link_libraries(gossip_local_subs_test
    p2p_gossip
    p2p_testutil_peer
    )
