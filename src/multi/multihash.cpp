/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multihash.hpp>

#include <boost/algorithm/hex.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/format.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/log/logger.hpp>

using libp2p::common::ByteArray;
using libp2p::common::hex_upper;
using libp2p::common::unhex;

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, Multihash::Error, e) {
  using E = libp2p::multi::Multihash::Error;
  switch (e) {
    case E::ZERO_INPUT_LENGTH:
      return "The length encoded in the header is zero";
    case E::INCONSISTENT_LENGTH:
      return "The length encoded in the input data header doesn't match the "
             "actual length of the input data";
    case E::INPUT_TOO_LONG:
      return "The length of the input exceeds the maximum length of "
          + std::to_string(libp2p::multi::Multihash::kMaxHashLength);
    case E::INPUT_TOO_SHORT:
      return "The length of the input is less than the required minimum of two "
             "bytes for the multihash header";
    default:
      return "Unknown error";
  }
}

namespace libp2p::multi {

  Multihash::Multihash(HashType type, gsl::span<const uint8_t> hash)
      : data_(std::make_shared<const Data>(type, hash)) {}

  Multihash::Data::Data(HashType t, gsl::span<const uint8_t> h) : type(t) {
    UVarint uvarint{type};
    auto &&varint_bytes = uvarint.toBytes();
    bytes.reserve(h.size() + 4);
    bytes.insert(bytes.end(), varint_bytes.begin(), varint_bytes.end());
    BOOST_ASSERT(h.size() <= std::numeric_limits<uint8_t>::max());
    bytes.push_back(static_cast<uint8_t>(h.size()));
    hash_offset = bytes.size();
    bytes.insert(bytes.end(), h.begin(), h.end());
    std_hash = boost::hash_range(bytes.begin(), bytes.end());
  }

  const Multihash::Data& Multihash::data() const {
#ifndef NDEBUG
    BOOST_ASSERT(data_);
#else
    if (data_ == nullptr) {
      log::createLogger("Multihash")->critical("attempt to use moved object");
      throw std::runtime_error("attempt to use moved multihash");
    }
#endif
    return *data_;
  }

  size_t Multihash::stdHash() const {
    return data().std_hash;
  }

  outcome::result<Multihash> Multihash::create(HashType type,
                                               gsl::span<const uint8_t> hash) {
    if (hash.size() > kMaxHashLength) {
      return Error::INPUT_TOO_LONG;
    }

    return Multihash{type, hash};
  }

  outcome::result<Multihash> Multihash::createFromHex(std::string_view hex) {
    OUTCOME_TRY(buf, unhex(hex));
    return Multihash::createFromBytes(buf);
  }

  outcome::result<Multihash> Multihash::createFromBytes(
      gsl::span<const uint8_t> b) {
    if (b.size() < kHeaderSize) {
      return Error::INPUT_TOO_SHORT;
    }

    UVarint varint(b);

    const auto type = static_cast<HashType>(varint.toUInt64());
    uint8_t length = b[varint.size()];
    gsl::span<const uint8_t> hash = b.subspan(varint.size() + 1);

    if (length == 0) {
      return Error::ZERO_INPUT_LENGTH;
    }

    if (hash.size() != length) {
      return Error::INCONSISTENT_LENGTH;
    }

    return Multihash::create(type, hash);
  }

  const HashType &Multihash::getType() const {
    return data().type;
  }

  gsl::span<const uint8_t> Multihash::getHash() const {
    return gsl::span<const uint8_t>(data().bytes).subspan(data().hash_offset);
  }

  std::string Multihash::toHex() const {
    return hex_upper(data().bytes);
  }

  const common::ByteArray &Multihash::toBuffer() const {
    return data().bytes;
  }

  bool Multihash::operator==(const Multihash &other) const {
    if (data_ == other.data_) {
      return true;
    }
    return data().bytes == other.data().bytes
        && data().type == other.data().type;
  }

  bool Multihash::operator!=(const Multihash &other) const {
    return !(this->operator==(other));
  }

  bool Multihash::operator<(const class libp2p::multi::Multihash &other) const {
    if (data().type == other.data().type) {
      return data().bytes < other.data().bytes;
    }
    return data().type < other.data().type;
  }

}  // namespace libp2p::multi
