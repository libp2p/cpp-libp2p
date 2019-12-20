/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/literals.hpp>
#include <libp2p/multi/multihash.hpp>
#include <libp2p/multi/uvarint.hpp>

using namespace libp2p::common;

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
  std::vector<uint8_t> hash{2, 3, 4};
  ASSERT_NO_THROW({
    auto m = Multihash::create(HashType::blake2s128, hash).value();
    ASSERT_EQ(m.getType(), HashType::blake2s128);
    ASSERT_EQ(m.getHash(), gsl::span<const uint8_t>(hash));
  });

  auto h = ByteArray(200, 42);
  ASSERT_FALSE(Multihash::create(HashType::blake2s128, h))
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
  ByteArray hash{2, 3, 4};

  ASSERT_NO_THROW({
    auto m = Multihash::create(HashType::blake2s128, hash).value();
    UVarint var(HashType::blake2s128);
    auto hex_s = hex_upper(var.toBytes()) + "03" + hex_upper(hash);
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
    auto m = Multihash::createFromBytes(hash).value();
    ASSERT_EQ(m.toBuffer(), hash);
  });

  ByteArray v{2, 3, 1, 3};
  ASSERT_FALSE(Multihash::createFromBytes(v))
      << "Length in the header does not equal actual length";
}

/**
 * @given blake hash and sha256 hash with same hash
 * @when compare multihashes
 * @then sha256 hash is less then blake hash
 */
TEST(Multihash, CompareDifferentTypes) {
  std::vector<uint8_t> hash{2, 3, 4};
  auto sha256_hash = Multihash::create(HashType::sha256, hash).value();
  auto blake_hash = Multihash::create(HashType::blake2s128, hash).value();
  // type sha256 < blake2s128
  ASSERT_TRUE(sha256_hash < blake_hash);
  ASSERT_FALSE(blake_hash < sha256_hash);
  ASSERT_FALSE(sha256_hash < sha256_hash);
  ASSERT_FALSE(blake_hash < blake_hash);
}

/**
 * @given similar hash type and different hashes
 * @when compare multihashes
 * @then lesser hash is less
 */
TEST(Multihash, CompareDifferentHashes) {
  std::vector<uint8_t> hash_lesser{2, 3, 4};
  std::vector<uint8_t> hash{3, 4, 5};
  auto hash1 = Multihash::create(HashType::sha256, hash_lesser).value();
  auto hash2 = Multihash::create(HashType::sha256, hash).value();
  ASSERT_TRUE(hash1 < hash2);
  ASSERT_FALSE(hash2 < hash1);
  ASSERT_FALSE(hash1 < hash1);
  ASSERT_FALSE(hash2 < hash2);
}
