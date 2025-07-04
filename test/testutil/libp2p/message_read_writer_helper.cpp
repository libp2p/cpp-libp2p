/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/libp2p/message_read_writer_helper.hpp"
#include "testutil/expect_read.hpp"
#include "testutil/expect_write.hpp"

#include <libp2p/multi/uvarint.hpp>

ACTION_P(PutBytes, bytes) {  // NOLINT
  std::copy(bytes.begin(), bytes.end(), arg0.begin());
  arg2(bytes.size());
}

ACTION_P(CheckBytes, bytes) {                                          // NOLINT
  ASSERT_EQ((std::vector<uint8_t>{arg0.begin(), arg0.end()}), bytes);  // NOLINT
  arg2(bytes.size());
}

namespace libp2p::basic {
  using multi::UVarint;
  using testing::_;

  void expect_read(auto &mock, BytesIn msg) {
    // read varint
    UVarint varint_to_read{msg.size()};
    auto &temp = EXPECT_CALL_READ(*mock);
    for (size_t i = 0; i < varint_to_read.size(); ++i) {
      temp.WILL_READ(BytesIn(varint_to_read.toVector()).subspan(i, 1));
    }

    // read message
    temp.WILL_READ(msg);
  }

  void expect_write(auto &mock, Bytes msg) {
    UVarint varint_to_write{msg.size()};
    msg.insert(msg.begin(),
               varint_to_write.toVector().begin(),
               varint_to_write.toVector().end());
    EXPECT_CALL_WRITE(*mock).WILL_WRITE(msg);
  }

  void setReadExpectations(
      const std::shared_ptr<ReadWriterMock> &read_writer_mock,
      const std::vector<uint8_t> &msg) {
    expect_read(read_writer_mock, msg);
  }

  void setReadExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      const std::vector<uint8_t> &msg) {
    expect_read(stream_mock, msg);
  }

  void setWriteExpectations(
      const std::shared_ptr<ReadWriterMock> &read_writer_mock,
      std::vector<uint8_t> msg) {
    expect_write(read_writer_mock, msg);
  }

  void setWriteExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      std::vector<uint8_t> msg) {
    expect_write(stream_mock, msg);
  }
}  // namespace libp2p::basic
