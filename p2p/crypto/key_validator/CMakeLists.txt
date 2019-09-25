#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(key_validator
    key_validator_impl.hpp
    key_validator_impl.cpp
    )
target_link_libraries(key_validator
    key_generator
    crypto_error
    OpenSSL::Crypto
    )
