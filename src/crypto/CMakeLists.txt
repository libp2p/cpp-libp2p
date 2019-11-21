#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(aes_provider)
add_subdirectory(hmac_provider)
add_subdirectory(crypto_provider)
add_subdirectory(key_marshaller)
add_subdirectory(key_validator)
add_subdirectory(protobuf)
add_subdirectory(random_generator)
add_subdirectory(sha)

libp2p_add_library(p2p_crypto_error
    error.cpp
    )
target_link_libraries(p2p_crypto_error
    Boost::boost
    )

libp2p_add_library(p2p_crypto_key
    key.cpp
    )
target_link_libraries(p2p_crypto_key
    Boost::boost
    )
