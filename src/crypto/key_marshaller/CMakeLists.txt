#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_key_marshaller
    key_marshaller_impl.cpp
    )
target_link_libraries(p2p_key_marshaller
    p2p_crypto_error
    p2p_crypto_key
    p2p_keys_proto
    )
