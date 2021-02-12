# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_inmem_address_repository
    inmem_address_repository.cpp
    )
target_link_libraries(p2p_inmem_address_repository
    p2p_address_repository
    p2p_multihash
    p2p_multiaddress
    p2p_peer_errors
    p2p_peer_id
    p2p_dnsaddr_resolver
    )
