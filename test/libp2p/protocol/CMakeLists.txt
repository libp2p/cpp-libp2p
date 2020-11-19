#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(common)
add_subdirectory(kademlia)
add_subdirectory(gossip)

addtest(identify_test
    identify_test.cpp
    )
target_link_libraries(identify_test
    p2p_identify
    p2p_literals
    )

addtest(identify_delta_test
    identify_delta_test.cpp
    )
target_link_libraries(identify_delta_test
    p2p_identify
    p2p_literals
    )

addtest(observed_addresses_test
    observed_addresses_test.cpp
    )
target_link_libraries(observed_addresses_test
    p2p_identify
    p2p_literals
    )

addtest(echo_test
    echo_test.cpp
    )
target_link_libraries(echo_test
    p2p_protocol_echo
    p2p_literals
    )

addtest(ping_test
    ping_test.cpp
    )
target_link_libraries(ping_test
    p2p_ping
    p2p_peer_id
    p2p_multiaddress
    p2p_literals
    )
