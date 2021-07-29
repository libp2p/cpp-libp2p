#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(message_read_writer_test
    message_read_writer_test.cpp
    )
target_link_libraries(message_read_writer_test
    p2p_message_read_writer
    p2p_protobuf_message_read_writer
    p2p_message_read_writer_error
    p2p_uvarint
    Boost::boost
    )

addtest(varint_prefix_reader_test
    varint_prefix_reader_test.cpp
    )
target_link_libraries(varint_prefix_reader_test
    p2p_varint_prefix_reader
    p2p_uvarint
    )

addtest(scheduler_test
    scheduler_test.cpp
    )
target_link_libraries(scheduler_test
    p2p_manual_scheduler_backend
    p2p_asio_scheduler_backend
    )
