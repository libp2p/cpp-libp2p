#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_sha
    sha256.cpp
    sha512.cpp
    )
target_link_libraries(p2p_sha
    PUBLIC
    p2p_crypto_error
    OpenSSL::SSL
    OpenSSL::Crypto
    )
