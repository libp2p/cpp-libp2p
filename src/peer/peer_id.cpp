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

  PeerId PeerId::fromPublicKey(const crypto::PublicKey &key) {
    auto hash = crypto::sha256(key.data);
    auto rmultihash = Multihash::create(multi::sha256, hash);
    BOOST_ASSERT(rmultihash.has_value());
    return PeerId{std::move(rmultihash.value())};
  }

  PeerId::FactoryResult PeerId::fromBase58(std::string_view id) {
    OUTCOME_TRY(decoded_id, decodeBase58(id));
    OUTCOME_TRY(hash, Multihash::createFromBuffer(decoded_id));

    if (hash.getType() != multi::HashType::sha256) {
      return FactoryError::SHA256_EXPECTED;
    }

    return PeerId{std::move(hash)};
  }

  PeerId::FactoryResult PeerId::fromHash(const Multihash &hash) {
    if (hash.getType() != multi::HashType::sha256) {
      return FactoryError::SHA256_EXPECTED;
    }

    return PeerId{hash};
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

  std::vector<uint8_t> PeerId::toVector() const {
    return hash_.toBuffer();
  }

  std::string PeerId::toHex() const {
    return hash_.toHex();
  }

  const multi::Multihash &PeerId::toMultihash() const {
    return hash_;
  }

  PeerId::FactoryResult PeerId::fromBytes(gsl::span<const uint8_t> v) {
    OUTCOME_TRY(mh, Multihash::createFromBuffer(v));
    return fromHash(mh);
  }
}  // namespace libp2p::peer

size_t std::hash<libp2p::peer::PeerId>::operator()(
    const libp2p::peer::PeerId &peer_id) const {
  return std::hash<libp2p::multi::Multihash>()(peer_id.toMultihash());
}
