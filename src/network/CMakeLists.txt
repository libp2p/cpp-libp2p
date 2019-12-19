#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

libp2p_add_library(p2p_default_network
    default_network.cpp
    )
target_link_libraries(p2p_default_network
    p2p_network
    p2p_tcp
    p2p_yamux
    p2p_mplex
    p2p_plaintext
    p2p_secio
    p2p_connection_manager
    p2p_transport_manager
    p2p_listener_manager
    p2p_identity_manager
    p2p_dialer
    p2p_router
    p2p_multiselect
    p2p_random_generator
    p2p_crypto_provider
    p2p_key_validator
    p2p_key_marshaller
    )
