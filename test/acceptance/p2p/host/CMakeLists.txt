# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(protocol)
add_subdirectory(peer)

addtest(basic_host_test
    basic_host_test.cpp
    )
target_link_libraries(basic_host_test
    p2p_basic_host
    p2p_peer_id
    p2p_multiaddress
    p2p_literals
    )

addtest(host_integration_test
    host_integration_test.cpp
    )
target_link_libraries(host_integration_test
    p2p_client_test_session
    p2p_default_network
    p2p_basic_host
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_protocol_echo
    p2p_multiaddress
    p2p_test_peer
    p2p_testutil
    p2p_testutil_peer
    p2p_literals
    )
