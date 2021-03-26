#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_multiselect
    protocol_muxer_error.cpp
    multiselect.cpp
    multiselect/multiselect_instance.cpp
    multiselect/parser.cpp
    multiselect/simple_stream_negotiate.cpp
    )
target_link_libraries(p2p_multiselect
    p2p_read_buffer
    p2p_varint_prefix_reader
    p2p_logger
    p2p_hexutil
    )


#add_library(p2p_protocol_muxer_error)
