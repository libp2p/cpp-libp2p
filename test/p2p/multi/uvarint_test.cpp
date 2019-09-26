/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <gsl/span>

#include "common/hexutil.hpp"
#include "libp2p/multi/uvarint.hpp"

using kagome::common::hex_upper;
using libp2p::multi::UVarint;

/**
 * @given an unsigned integer
 * @when creating a varint from the integer
 * @then a valid varint is created, which yields the original integer when
 * decoded
 */
TEST(UVarint, CreateFromInt) {
  UVarint v(2);
  ASSERT_EQ(v.toUInt64(), 2);
  v = 3245;
  ASSERT_EQ(v.toUInt64(), 3245);
  v = 0;
  ASSERT_EQ(v.toUInt64(), 0);
}

/**
 * @given an unsigned integer
 * @when creating a varint from the integer
 * @then a valid varint is created, which binary representation corresponds to
 * the varint standard
 */
TEST(UVarint, CorrectEncoding) {
  UVarint var(1);
  ASSERT_EQ(hex_upper(var.toBytes()), "01");
  var = 127;
  ASSERT_EQ(hex_upper(var.toBytes()), "7F");
  var = 128;
  ASSERT_EQ(hex_upper(var.toBytes()), "8001");
  var = 255;
  ASSERT_EQ(hex_upper(var.toBytes()), "FF01");
  var = 300;
  ASSERT_EQ(hex_upper(var.toBytes()), "AC02");
  var = 16384;
  ASSERT_EQ(hex_upper(var.toBytes()), "808001");
}

/**
 * @given a byte array
 * @when assessing the size of a varint stored in the array
 * @then result is the size corresponding to the varint standard
 */
TEST(UVarint, CalculateSize) {
  uint8_t bytes[] = {0x81, 0xA3, 0x75, 0x43, 0xAA};
  ASSERT_EQ(UVarint::calculateSize(gsl::span(bytes, 5)), 3);

  uint8_t another_bytes[] = {0x71, 0xA3, 0x75, 0x43, 0xAA};
  ASSERT_EQ(UVarint::calculateSize(gsl::span(another_bytes, 5)), 1);
}
