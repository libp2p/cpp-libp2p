# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_inmem_key_repository
    inmem_key_repository.cpp
    )
target_link_libraries(libp2p_inmem_key_repository
    libp2p_peer_errors
    multihash
    crypto_key
    libp2p_peer_id
    )
