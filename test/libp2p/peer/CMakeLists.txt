#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(peer_id_test
    peer_id_test.cpp
    )
target_link_libraries(peer_id_test
    p2p_peer_id
    p2p_crypto_provider
    p2p_key_marshaller
    p2p_random_generator
    p2p_key_validator
    )

addtest(peer_address_test
    peer_address_test.cpp
    )
target_link_libraries(peer_address_test
    p2p_peer_address
    )

add_subdirectory(address_repository)
add_subdirectory(key_book)
add_subdirectory(protocol_repository)
