# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(basic_host)

libp2p_add_library(p2p_default_host
    default_host.cpp
    )
target_link_libraries(p2p_default_host
    p2p_basic_host
    p2p_inmem_key_repository
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_protocol_repository
    )
