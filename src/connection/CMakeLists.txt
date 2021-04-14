#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_connection_error
    error_codes.cpp
    )
target_link_libraries(p2p_connection_error
    p2p_logger
    )

add_library(p2p_loopback_stream
    loopback_stream.cpp
    )
target_link_libraries(p2p_loopback_stream
    p2p_connection_error
    p2p_peer_id
    )

libp2p_install(p2p_loopback_stream)

