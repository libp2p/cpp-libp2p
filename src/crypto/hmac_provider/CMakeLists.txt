#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_hmac_provider
    hmac_provider_impl.cpp
    hmac_provider_ctr_impl.cpp
    )
target_link_libraries(p2p_hmac_provider
    p2p_crypto_error
    OpenSSL::Crypto
    )
