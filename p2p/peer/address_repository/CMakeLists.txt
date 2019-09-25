# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_inmem_address_repository
    inmem_address_repository.cpp
    )
target_link_libraries(libp2p_inmem_address_repository
    address_repository
    multihash
    multiaddress
    libp2p_peer_errors
    libp2p_peer_id
    )
