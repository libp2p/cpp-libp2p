/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOBUF_MESSAGE_MOCK_HPP
#define LIBP2P_PROTOBUF_MESSAGE_MOCK_HPP

#include <gmock/gmock.h>

/**
 * Basic interface of Protobuf message with a subset of methods, which are
 * inside of the real class
 */
struct ProtobufMessage {
  virtual ~ProtobufMessage() = default;

  virtual bool ParseFromArray(const void *data, int size) = 0;

  virtual bool SerializeToArray(void *data, int size) const = 0;

  virtual int ByteSize() const = 0;
};

class ProtobufMessageMock : public ProtobufMessage {
 public:
  ~ProtobufMessageMock() override = default;

  MOCK_METHOD2(ParseFromArray, bool(const void *, int));

  MOCK_CONST_METHOD2(SerializeToArray, bool(void *, int));

  MOCK_CONST_METHOD0(ByteSize, int());
};

#endif  // LIBP2P_PROTOBUF_MESSAGE_MOCK_HPP
