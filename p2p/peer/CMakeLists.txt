#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(address_repository)
add_subdirectory(key_repository)
add_subdirectory(protocol_repository)
add_subdirectory(impl)

add_library(address_repository
    address_repository.cpp
    )
target_link_libraries(address_repository
    Boost::boost
    )

add_library(libp2p_peer_errors
    errors.cpp
    )
target_link_libraries(libp2p_peer_errors
    outcome
    )

add_library(libp2p_peer_id
    peer_id.cpp
    peer_id.hpp
    )
target_link_libraries(libp2p_peer_id
    outcome
    multihash
    multibase_codec
    hasher
    )

add_library(libp2p_peer_address
    peer_address.cpp
    peer_address.hpp
    )
target_link_libraries(libp2p_peer_address
    outcome
    multiaddress
    multihash
    multibase_codec
    libp2p_peer_id
    )
