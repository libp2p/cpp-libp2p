# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(test_peer
    test_peer.hpp
    test_peer.cpp
    )
target_link_libraries(test_peer
    Boost::boost
    libp2p_basic_host
    libp2p_peer_repository
    libp2p_inmem_address_repository
    libp2p_inmem_key_repository
    libp2p_inmem_protocol_repository
    libp2p_protocol_echo
    client_test_session
    )
