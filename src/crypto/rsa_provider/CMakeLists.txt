#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_rsa
    rsa_provider_impl.cpp
    )

target_link_libraries(p2p_rsa
    p2p_sha
    p2p_crypto_error
    OpenSSL::Crypto
    )
