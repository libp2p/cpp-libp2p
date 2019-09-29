#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_sha
    sha256.cpp
    )
target_link_libraries(p2p_sha
    PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    )
