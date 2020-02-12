#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(host)

addtest(all_muxers_acceptance_test
    muxer.cpp
    )
target_link_libraries(all_muxers_acceptance_test
    p2p_yamux
    p2p_mplex
    p2p_multiaddress
    p2p_plaintext
    p2p_tcp
    p2p_testutil
    p2p_key_marshaller
    p2p_identity_manager
    p2p_literals
    )

addtest(protocol_streams_regression protocol_streams_regression.cpp)

target_link_libraries(protocol_streams_regression
    p2p_basic_host
    p2p_default_network
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    )

