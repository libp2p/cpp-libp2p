# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_inmem_protocol_repository
    inmem_protocol_repository.cpp
    )
target_link_libraries(libp2p_inmem_protocol_repository
    outcome
    libp2p_peer_errors
    multihash
    multistream
    libp2p_peer_id
    )
