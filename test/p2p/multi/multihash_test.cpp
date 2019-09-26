/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "common/hexutil.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/multi/uvarint.hpp"

using kagome::common::Buffer;
using kagome::common::hex_upper;

using libp2p::multi::HashType;
using libp2p::multi::Multihash;
using libp2p::multi::UVarint;

/**
 *   @given a buffer with a hash
 *   @when creating a multihash using the buffer
 *   @then a correct multihash object is created if the hash size is not greater
 *         than maximum length
 **/
TEST(Multihash, Create) {
  Buffer hash{2, 3, 4};
  ASSERT_NO_THROW({
    auto m = Multihash::create(HashType::blake2s128, hash).value();
    ASSERT_EQ(m.getType(), HashType::blake2s128);
    ASSERT_EQ(m.getHash(), hash);
  });

  ASSERT_FALSE(Multihash::create(HashType::blake2s128, Buffer(200, 42)))
      << "The multihash mustn't accept hashes of the size greater than 127";
}

/**
 *   @given a buffer with a hash or a hex string with a hash
 *   @when creating a multihash from them
 *   @then a correct multihash object is created if the given hash object was
 *         valid, and the hex representation of the created multihash matches
 *the given hash string
 **/
TEST(Multihash, FromToHex) {
  Buffer hash{2, 3, 4};

  ASSERT_NO_THROW({
    auto m = Multihash::create(HashType::blake2s128, hash).value();
    UVarint var(HashType::blake2s128);
    auto hex_s = hex_upper(var.toBytes()) + "03" + hex_upper(hash.toVector());
    ASSERT_EQ(m.toHex(), hex_s);
  });

  ASSERT_NO_THROW({
    auto m = "1203020304"_multihash;
    ASSERT_EQ(m.toHex(), "1203020304");
  });

  ASSERT_FALSE(Multihash::createFromHex("32004324234234"))
      << "The length mustn't be zero";
  ASSERT_FALSE(Multihash::createFromHex("32034324234234"))
      << "The length must be equal to the hash size";
  ASSERT_FALSE(Multihash::createFromHex("3204abcdefgh"))
      << "The hex string is invalid";
}

/**
 *   @given a multihash or a buffer
 *   @when converting a multihash to a buffer or creating one from a buffer
 *   @then a correct multihash object is created if the hash size is not greater
 *         than maximum length or correct buffer object representing the
 *multihash is returned
 **/
TEST(Multihash, FromToBuffer) {
  auto hash = "8203020304"_unhex;

  ASSERT_NO_THROW({
    auto m = Multihash::createFromBuffer(hash).value();
    ASSERT_EQ(m.toBuffer(), hash);
  });

  std::vector<uint8_t> v{2, 3, 1, 3};
  ASSERT_FALSE(Multihash::createFromBuffer(v))
      << "Length in the header does not equal actual length";
}
