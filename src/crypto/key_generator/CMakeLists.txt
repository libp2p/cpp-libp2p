#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_key_generator
    key_generator_impl.cpp
    )

target_link_libraries(p2p_key_generator
    p2p_crypto_error
    p2p_random_generator
    OpenSSL::Crypto
    Boost::filesystem
    )
