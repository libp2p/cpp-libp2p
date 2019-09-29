# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(test_peer
    test_peer.hpp
    test_peer.cpp
    )
target_link_libraries(test_peer
    Boost::boost
    p2p_basic_host
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_protocol_echo
    client_test_session
    )
