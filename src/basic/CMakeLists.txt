#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_varint_reader
    varint_reader.cpp
    )
target_link_libraries(p2p_varint_reader
    p2p_uvarint
    )

libp2p_add_library(p2p_varint_prefix_reader
    varint_prefix_reader.cpp
    )
target_link_libraries(p2p_varint_prefix_reader
    p2p_logger
    )

libp2p_add_library(p2p_message_read_writer_error
    message_read_writer_error.cpp
    )
target_link_libraries(p2p_message_read_writer_error
    Boost::boost
    )

libp2p_add_library(p2p_message_read_writer
    message_read_writer_bigendian.cpp
    message_read_writer_uvarint.cpp
    )
target_link_libraries(p2p_message_read_writer
    p2p_message_read_writer_error
    p2p_varint_reader
    )

libp2p_add_library(p2p_protobuf_message_read_writer
    protobuf_message_read_writer.cpp
    )
target_link_libraries(p2p_protobuf_message_read_writer
    p2p_message_read_writer
    )

libp2p_add_library(p2p_read_buffer
    read_buffer.cpp
    )
target_link_libraries(p2p_read_buffer
    p2p_logger
    )

libp2p_add_library(p2p_write_queue
    write_queue.cpp
    )
target_link_libraries(p2p_write_queue
    p2p_logger
    )

libp2p_add_library(p2p_basic_scheduler
    scheduler.cpp
    scheduler/scheduler_impl.cpp
    )
target_link_libraries(p2p_basic_scheduler
    p2p_logger
    )

libp2p_add_library(p2p_asio_scheduler_backend
    scheduler/asio_scheduler_backend.cpp
    )
target_link_libraries(p2p_asio_scheduler_backend
    p2p_basic_scheduler
    )

libp2p_add_library(p2p_manual_scheduler_backend
    scheduler/manual_scheduler_backend.cpp
    )
target_link_libraries(p2p_manual_scheduler_backend
    p2p_basic_scheduler
    )

