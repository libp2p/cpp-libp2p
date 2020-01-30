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

  Multihash::Multihash(HashType type, gsl::span<const uint8_t> hash) {
    type_ = type;
    UVarint uvarint{type};
    auto &&bytes = uvarint.toBytes();
    data_.insert(data_.end(), bytes.begin(), bytes.end());
    BOOST_ASSERT(hash.size() <= std::numeric_limits<uint8_t>::max());
    data_.push_back(static_cast<uint8_t>(hash.size()));
    hash_offset_ = data_.size();
    data_.insert(data_.end(), hash.begin(), hash.end());
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
    return type_;
  }

  gsl::span<const uint8_t> Multihash::getHash() const {
    return gsl::span<const uint8_t>(data_).subspan(hash_offset_);
  }

  std::string Multihash::toHex() const {
    return hex_upper(data_);
  }

  const common::ByteArray &Multihash::toBuffer() const {
    return data_;
  }

  bool Multihash::operator==(const Multihash &other) const {
    return this->data_ == other.data_ && this->type_ == other.type_;
  }

  bool Multihash::operator!=(const Multihash &other) const {
    return !(this->operator==(other));
  }

  bool Multihash::operator<(const class libp2p::multi::Multihash &other) const {
    return this->type_ < other.type_
        || (this->type_ == other.type_ && this->data_ < other.data_);
  }

}  // namespace libp2p::multi

size_t std::hash<libp2p::multi::Multihash>::operator()(
    const libp2p::multi::Multihash &x) const {
  const auto &container = x.toBuffer();
  return boost::hash_range(container.begin(), container.end());
}
