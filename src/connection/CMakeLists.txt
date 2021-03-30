#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_loopback_stream
    loopback_stream.cpp
    )
target_link_libraries(p2p_loopback_stream
    p2p_logger
    p2p_peer_id
    )

libp2p_install(p2p_loopback_stream)
