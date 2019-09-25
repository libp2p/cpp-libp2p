#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(key_generator
    key_generator_impl.hpp
    key_generator_impl.cpp
    )

target_link_libraries(key_generator
    crypto_error
    random_generator
    OpenSSL::Crypto
    Boost::filesystem
    )
