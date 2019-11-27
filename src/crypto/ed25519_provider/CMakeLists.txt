#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_ed25519_provider
    ed25519_provider_impl.cpp
    )

target_link_libraries(p2p_ed25519_provider
    p2p_crypto_error
    p2p_crypto_common
    p2p_sha
    OpenSSL::Crypto
    )
