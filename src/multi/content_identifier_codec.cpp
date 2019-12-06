/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/multicodec_type.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/crypto/sha/sha256.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, ContentIdentifierCodec::EncodeError,
                            e) {
  using E = libp2p::multi::ContentIdentifierCodec::EncodeError;
  switch (e) {
    case E::INVALID_CONTENT_TYPE:
      return "Content type does not conform the version";
    case E::INVALID_HASH_LENGTH:
      return "Hash length is invalid; Must be 32 bytes for sha256 in version 0";
    case E::INVALID_HASH_TYPE:
      return "Hash type is invalid; Must be sha256 in version 0";
  }
  return "Unknown error";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, ContentIdentifierCodec::DecodeError,
                            e) {
  using E = libp2p::multi::ContentIdentifierCodec::DecodeError;
  switch (e) {
    case E::EMPTY_MULTICODEC:
      return "Multicodec prefix is absent";
    case E::EMPTY_VERSION:
      return "Version is absent";
    case E::MALFORMED_VERSION:
      return "Version is malformed; Must be a non-negative integer";
    case E::RESERVED_VERSION:
      return "Version is greater than the latest version";
  }
  return "Unknown error";
}

namespace libp2p::multi {

  outcome::result<std::vector<uint8_t>> ContentIdentifierCodec::encode(
      const ContentIdentifier &cid) {
    std::vector<uint8_t> bytes;
    if (cid.version == ContentIdentifier::Version::V1) {
      UVarint version(static_cast<uint64_t>(cid.version));
      common::append(bytes, version.toBytes());
      UVarint type(cid.content_type);
      common::append(bytes, type.toBytes());
      auto const &hash = cid.content_address.toBuffer();
      common::append(bytes, hash);

    } else if (cid.version == ContentIdentifier::Version::V0) {
      if (cid.content_type != MulticodecType::DAG_PB) {
        return EncodeError::INVALID_CONTENT_TYPE;
      }
      if (cid.content_address.getType() != HashType::sha256) {
        return EncodeError::INVALID_HASH_TYPE;
      }
      if (cid.content_address.getHash().size() != 32) {
        return EncodeError::INVALID_HASH_LENGTH;
      }
      auto const &hash = cid.content_address.toBuffer();
      common::append(bytes, hash);
    }
    return bytes;
  }

  std::vector<uint8_t> ContentIdentifierCodec::encodeCIDV0(
      const void* byte_buffer, size_t sz) {
    std::vector<uint8_t> bytes;
    bytes.resize(34);
    bytes[0] = 0x12; // sha256 hash type
    bytes[1] = 0x20; // hash length
    auto hash = crypto::sha256(gsl::span<uint8_t>(
        (uint8_t*)byte_buffer, //NOLINT
        sz));
    memcpy(&bytes[2], hash.data(), 0x20);
    return bytes;
  }

  outcome::result<ContentIdentifier> ContentIdentifierCodec::decode(
      gsl::span<const uint8_t> bytes) {
    if (bytes.size() == 34 and bytes[0] == 0x12 and bytes[1] == 0x20) {
      OUTCOME_TRY(hash, Multihash::createFromBytes(bytes));
      return ContentIdentifier(ContentIdentifier::Version::V0,
                               MulticodecType::DAG_PB, std::move(hash));
    }
    auto version_opt = UVarint::create(bytes);
    if (!version_opt) {
      return DecodeError::EMPTY_VERSION;
    }
    auto version = version_opt.value().toUInt64();
    if (version == 1) {
      auto version_length = UVarint::calculateSize(bytes);
      auto multicodec_opt = UVarint::create(bytes.subspan(version_length));
      if (!multicodec_opt) {
        return DecodeError::EMPTY_MULTICODEC;
      }
      auto multicodec_length =
          UVarint::calculateSize(bytes.subspan(version_length));
      OUTCOME_TRY(hash,
                  Multihash::createFromBytes(
                      bytes.subspan(version_length + multicodec_length)));
      return ContentIdentifier(
          ContentIdentifier::Version::V1,
          MulticodecType::Code(multicodec_opt.value().toUInt64()),
          std::move(hash));
    }
    if (version <= 0) {
      return DecodeError::MALFORMED_VERSION;
    }
    return DecodeError::RESERVED_VERSION;
  }
}  // namespace libp2p::multi
