#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_varint_reader
    varint_reader.cpp
    )
target_link_libraries(p2p_varint_reader
    p2p_uvarint
    )

add_library(p2p_message_read_writer_error
    message_read_writer_error.cpp
    )
target_link_libraries(p2p_message_read_writer_error
    outcome
    )

add_library(p2p_message_read_writer
    message_read_writer.cpp
    )
target_link_libraries(p2p_message_read_writer
    p2p_message_read_writer_error
    p2p_varint_reader
    )

add_library(p2p_protobuf_message_read_writer
    protobuf_message_read_writer.cpp
    )
target_link_libraries(p2p_protobuf_message_read_writer
    p2p_message_read_writer
    )
