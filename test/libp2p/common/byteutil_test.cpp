/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/byteutil.hpp>

#include <gtest/gtest.h>

using namespace libp2p::common;

/**
 * Tests integers to little endian serializer.
 * The test has to pass on both types of hosts - little endian and big endian.
 * @given a 64-bit integer
 * @when it gets split into bytes in little endian order
 * @then the bytes and the order are equal to expected
 */
TEST(Byteutil, ToLittleEndian) {
  uint64_t value = 11578966645329144507ull;
  ASSERT_EQ(value, 0xa0b0c0d0e0f0aabb);
  std::vector<uint8_t> bytes;
  bytes.reserve(sizeof(uint64_t));
  putUint64LE(bytes, value);

  std::vector<uint8_t> expected = {0xbb, 0xaa, 0xf0, 0xe0,
                                   0xd0, 0xc0, 0xb0, 0xa0};
  ASSERT_EQ(bytes, expected);
}
