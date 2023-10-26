#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_executable(example_ares_resolver
    ares_resolver.cpp
    )
target_link_libraries(example_ares_resolver
    Boost::Boost.DI
    p2p_basic_host
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_default_network
    p2p_cares
    )
