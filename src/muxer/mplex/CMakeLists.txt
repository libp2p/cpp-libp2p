#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_mplex
    mplex.cpp
    )
target_link_libraries(p2p_mplex
    p2p_mplexed_connection
    )

libp2p_add_library(p2p_mplexed_connection
    mplexed_connection.cpp
    mplex_frame.cpp
    mplex_stream.cpp
    )
target_link_libraries(p2p_mplexed_connection
    p2p_logger
    p2p_uvarint
    p2p_varint_reader
    p2p_connection_error
    )
