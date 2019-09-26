# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(protocol)
add_subdirectory(peer)

addtest(basic_host_test
    basic_host_test.cpp
    )
target_link_libraries(basic_host_test
    libp2p_basic_host
    libp2p_peer_id
    multiaddress
    )

addtest(host_integration_test
    host_integration_test.cpp
    )
target_link_libraries(host_integration_test
    client_test_session
    libp2p_default_network
    libp2p_basic_host
    libp2p_peer_repository
    libp2p_inmem_address_repository
    libp2p_inmem_key_repository
    libp2p_inmem_protocol_repository
    libp2p_protocol_echo
    multiaddress
    test_peer
    testutil
    testutil_peer
    )
