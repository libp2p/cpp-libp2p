#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(address_repository)
add_subdirectory(key_repository)
add_subdirectory(protocol_repository)
add_subdirectory(impl)

add_library(p2p_address_repository
    address_repository.cpp
    )
target_link_libraries(p2p_address_repository
    Boost::boost
    )

add_library(p2p_peer_errors
    errors.cpp
    )
target_link_libraries(p2p_peer_errors
    outcome
    )

add_library(p2p_peer_id
    peer_id.cpp
    )
target_link_libraries(p2p_peer_id
    outcome
    p2p_multihash
    p2p_multibase_codec
    p2p_sha
    )

add_library(p2p_peer_address
    peer_address.cpp
    )
target_link_libraries(p2p_peer_address
    outcome
    p2p_multiaddress
    p2p_multihash
    p2p_multibase_codec
    p2p_peer_id
    )
