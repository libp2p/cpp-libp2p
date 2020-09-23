#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_tls
    tls_adaptor.cpp
    tls_connection.cpp
    tls_details.cpp
    )
target_link_libraries(p2p_tls
    Boost::boost
    p2p_crypto_error
    p2p_logger
    p2p_security_error
    )
