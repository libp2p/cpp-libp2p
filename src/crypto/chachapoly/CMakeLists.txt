#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_chachapoly_provider
    chachapoly_impl.cpp
    )

target_link_libraries(p2p_chachapoly_provider
    p2p_crypto_error
    p2p_byteutil
    p2p_logger
    OpenSSL::Crypto
    )
