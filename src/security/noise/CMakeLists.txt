#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

libp2p_add_library(p2p_noise
    noise.cpp
    noise_connection.cpp
    handshake.cpp
    crypto/state.cpp
    crypto/hkdf.cpp
    crypto/noise_dh.cpp
    crypto/noise_sha256.cpp
    crypto/noise_ccp1305.cpp
    crypto/cipher_suite.cpp
    insecure_rw.cpp
    )
target_link_libraries(p2p_noise
    Boost::boost
    p2p_noise_handshake_message_marshaller
    p2p_x25519_provider
    p2p_hmac_provider
    p2p_chachapoly_provider
    p2p_hexutil
    )

libp2p_add_library(p2p_noise_handshake_message_marshaller
    handshake_message_marshaller_impl.cpp
    )
target_link_libraries(p2p_noise_handshake_message_marshaller
    Boost::boost
    p2p_noise_proto
    p2p_key_marshaller
    )
