#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_identity_manager
    identity_manager_impl.cpp
    )
target_link_libraries(p2p_identity_manager
    p2p_peer_id
    )

libp2p_add_library(p2p_peer_repository
    peer_repository_impl.cpp
    )
target_link_libraries(p2p_peer_repository
    p2p_address_repository
    p2p_peer_id
    )
