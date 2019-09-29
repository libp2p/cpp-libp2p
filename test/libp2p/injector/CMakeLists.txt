# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(network_injector_test
    network_injector_test.cpp
    )
target_link_libraries(network_injector_test
    libp2p_default_network
    )


addtest(host_injector_test
    host_injector_test.cpp
    )
target_link_libraries(host_injector_test
    libp2p_default_network
    libp2p_basic_host
    libp2p_peer_repository
    libp2p_inmem_address_repository
    libp2p_inmem_key_repository
    libp2p_inmem_protocol_repository
    )
