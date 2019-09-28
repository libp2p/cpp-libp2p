#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(libp2p_default_network
    default_network.cpp
    )
target_link_libraries(libp2p_default_network
    libp2p_network
    libp2p_tcp
    libp2p_yamux
    libp2p_plaintext
    libp2p_connection_manager
    libp2p_transport_manager
    libp2p_listener_manager
    libp2p_identity_manager
    libp2p_dialer
    libp2p_router
    multiselect
    random_generator
    key_generator
    key_validator
    key_marshaller
    )
