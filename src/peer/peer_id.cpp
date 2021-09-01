/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/peer_id.hpp>

#include <boost/assert.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerId::FactoryError, e) {
  using E = libp2p::peer::PeerId::FactoryError;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::SHA256_EXPECTED:
      return "expected a sha-256 multihash";
  }
  return "unknown error";
}

namespace libp2p::peer {
  using common::ByteArray;
  using multi::Multihash;
  using multi::detail::decodeBase58;
  using multi::detail::encodeBase58;

  PeerId::PeerId(multi::Multihash hash) : hash_{std::move(hash)} {}

  PeerId::FactoryResult PeerId::fromPublicKey(const crypto::ProtobufKey &key) {
    std::vector<uint8_t> hash;

    auto algo = multi::sha256;
    if (key.key.size() <= kMaxInlineKeyLength) {
      algo = multi::identity;
      hash = key.key;
    } else {
      OUTCOME_TRY(shash, crypto::sha256(key.key));
      hash = std::vector<uint8_t>{shash.begin(), shash.end()};
    }

    OUTCOME_TRY(multihash, Multihash::create(algo, hash));
    return PeerId{std::move(multihash)};
  }

  PeerId::FactoryResult PeerId::fromBase58(std::string_view id) {
    OUTCOME_TRY(decoded_id, decodeBase58(id));
    OUTCOME_TRY(hash, Multihash::createFromBytes(decoded_id));

    if (hash.getType() != multi::HashType::sha256
        && hash.toBuffer().size() > kMaxInlineKeyLength) {
      return FactoryError::SHA256_EXPECTED;
    }

    return PeerId{std::move(hash)};
  }

  PeerId::FactoryResult PeerId::fromHash(const Multihash &hash) {
    if (hash.getType() != multi::HashType::sha256
        && hash.toBuffer().size() > kMaxInlineKeyLength) {
      return FactoryError::SHA256_EXPECTED;
    }

    return PeerId{hash};
  }

  bool PeerId::operator<(const PeerId &other) const {
    return this->hash_ < other.hash_;
  }

  bool PeerId::operator==(const PeerId &other) const {
    return this->hash_ == other.hash_;
  }

  bool PeerId::operator!=(const PeerId &other) const {
    return !(*this == other);
  }

  std::string PeerId::toBase58() const {
    return encodeBase58(hash_.toBuffer());
  }

  const std::vector<uint8_t> &PeerId::toVector() const {
    return hash_.toBuffer();
  }

  std::string PeerId::toHex() const {
    return hash_.toHex();
  }

  const multi::Multihash &PeerId::toMultihash() const {
    return hash_;
  }

  PeerId::FactoryResult PeerId::fromBytes(gsl::span<const uint8_t> v) {
    OUTCOME_TRY(mh, Multihash::createFromBytes(v));
    return fromHash(mh);
  }
}  // namespace libp2p::peer

size_t std::hash<libp2p::peer::PeerId>::operator()(
    const libp2p::peer::PeerId &peer_id) const {
  return std::hash<libp2p::multi::Multihash>()(peer_id.toMultihash());
}
