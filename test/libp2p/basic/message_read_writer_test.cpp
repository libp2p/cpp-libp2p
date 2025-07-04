/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/message_read_writer_uvarint.hpp>

#include <gtest/gtest.h>
#include <libp2p/multi/uvarint.hpp>
#include "mock/libp2p/connection/layer_connection_mock.hpp"
#include "testutil/expect_read.hpp"
#include "testutil/expect_write.hpp"
#include "testutil/gmock_actions.hpp"

using namespace libp2p;
using namespace basic;
using namespace connection;
using namespace multi;

using testing::_;
using testing::Return;

class MessageReadWriterTest : public testing::Test {
 public:
  std::shared_ptr<LayerConnectionMock> conn_mock_ =
      std::make_shared<LayerConnectionMock>();

  std::shared_ptr<MessageReadWriter> msg_rw_ =
      std::make_shared<MessageReadWriterUvarint>(conn_mock_);

  static constexpr uint64_t kMsgLength = 4;
  UVarint len_varint_ = UVarint{kMsgLength};

  Bytes msg_bytes_{0x11, 0x22, 0x33, 0x44};

  Bytes msg_with_varint_bytes_ = [this]() -> Bytes {
    Bytes buffer = len_varint_.toVector();
    buffer.insert(buffer.cend(), msg_bytes_.begin(), msg_bytes_.end());
    return buffer;
  }();

  bool operation_completed_ = false;
};

TEST_F(MessageReadWriterTest, Read) {
  EXPECT_CALL_READ(*conn_mock_)
      .WILL_READ(len_varint_.toBytes())
      .WILL_READ(msg_bytes_);

  msg_rw_->read([this](auto &&res) {
    ASSERT_TRUE(res);
    ASSERT_EQ(*res.value(), msg_bytes_);
    operation_completed_ = true;
  });

  ASSERT_TRUE(operation_completed_);
}

TEST_F(MessageReadWriterTest, Write) {
  EXPECT_CALL_WRITE(*conn_mock_).WILL_WRITE(msg_with_varint_bytes_);

  msg_rw_->write(msg_bytes_, [this](auto &&res) {
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), msg_bytes_.size());
    operation_completed_ = true;
  });

  ASSERT_TRUE(operation_completed_);
}
