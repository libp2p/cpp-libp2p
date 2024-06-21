#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_quic
    connection.cpp
    engine.cpp
    listener.cpp
    stream.cpp
    transport.cpp
    )
target_link_libraries(p2p_quic
    lsquic::lsquic
    p2p_tls
    )
