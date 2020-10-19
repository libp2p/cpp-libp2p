#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(noise)
add_subdirectory(plaintext)
add_subdirectory(secio)
add_subdirectory(tls)

libp2p_add_library(p2p_security_error
    error.cpp
    )
target_link_libraries(p2p_security_error
    Boost::boost
    )
