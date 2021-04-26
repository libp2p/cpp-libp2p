# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(network_injector_test
    network_injector_test.cpp
    )
target_link_libraries(network_injector_test
    Boost::Boost.DI
    p2p_default_network
    p2p_cares
    )


addtest(host_injector_test
    host_injector_test.cpp
    )
target_link_libraries(host_injector_test
    Boost::Boost.DI
    p2p_default_network
    p2p_basic_host
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    )
