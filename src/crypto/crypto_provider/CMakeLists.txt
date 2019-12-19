#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_crypto_provider
        crypto_provider_impl.cpp
    )

target_link_libraries(p2p_crypto_provider
    p2p_aes_provider
    p2p_crypto_error
    p2p_ed25519_provider
    p2p_hmac_provider
    p2p_random_generator
    OpenSSL::Crypto
    Boost::filesystem
    )
