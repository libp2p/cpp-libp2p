#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(varint_reader
    varint_reader.cpp
    )
target_link_libraries(varint_reader
    uvarint
    )

add_library(message_read_writer_error
    message_read_writer_error.cpp
    )
target_link_libraries(message_read_writer_error
    outcome
    )

add_library(message_read_writer
    message_read_writer.cpp
    )
target_link_libraries(message_read_writer
    message_read_writer_error
    varint_reader
    )

add_library(protobuf_message_read_writer
    protobuf_message_read_writer.cpp
    )
target_link_libraries(protobuf_message_read_writer
    message_read_writer
    )
