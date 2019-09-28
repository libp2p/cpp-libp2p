#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(aes_provider
    aes_provider_impl.cpp
    )

target_link_libraries(aes_provider
    crypto_error
    OpenSSL::Crypto
    )
