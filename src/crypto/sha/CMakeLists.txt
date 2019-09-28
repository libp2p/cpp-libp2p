#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sha
    sha256.cpp
    )
target_link_libraries(sha
    PUBLIC OpenSSL::SSL
    OpenSSL::Crypto
    )
