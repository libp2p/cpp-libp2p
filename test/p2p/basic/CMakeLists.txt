#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(message_read_writer_test
    message_read_writer_test.cpp
    )
target_link_libraries(message_read_writer_test
    message_read_writer
    protobuf_message_read_writer
    message_read_writer_error
    uvarint
    outcome
    buffer
    )
