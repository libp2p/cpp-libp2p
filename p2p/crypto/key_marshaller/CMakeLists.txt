#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(key_marshaller
    key_marshaller_impl.hpp
    key_marshaller_impl.cpp
    )
target_link_libraries(key_marshaller
    crypto_error
    crypto_key
    keys_proto
    )
