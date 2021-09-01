/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>
#include <libp2p/multi/multicodec_type.hpp>
#include <libp2p/multi/uvarint.hpp>

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
    case E::INVALID_BASE_ENCODING:
      return "Invalid base encoding";
    case E::VERSION_UNSUPPORTED:
      return "Content identifier version unsupported";
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
    case E::CID_TOO_SHORT:
      return "CID too short";
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
      UVarint type(static_cast<uint64_t>(cid.content_type));
      common::append(bytes, type.toBytes());
      auto const &hash = cid.content_address.toBuffer();
      common::append(bytes, hash);

    } else if (cid.version == ContentIdentifier::Version::V0) {
      if (cid.content_type != MulticodecType::Code::DAG_PB) {
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
      const void *byte_buffer, size_t sz) {
    auto digest_res = crypto::sha256(gsl::make_span(
        reinterpret_cast<const uint8_t *>(byte_buffer), sz));  // NOLINT
    BOOST_ASSERT(digest_res.has_value());

    auto &hash = digest_res.value();

    std::vector<uint8_t> bytes;
    bytes.reserve(hash.size());
    bytes.push_back(static_cast<uint8_t>(HashType::sha256));
    bytes.push_back(hash.size());
    std::copy(hash.begin(), hash.end(), std::back_inserter(bytes));
    return bytes;
  }

  std::vector<uint8_t> ContentIdentifierCodec::encodeCIDV1(
      MulticodecType::Code content_type, const Multihash &mhash) {
    std::vector<uint8_t> bytes;
    // Reserve space for CID version size + content-type size + multihash size
    bytes.reserve(1 + 1 + mhash.toBuffer().size());
    bytes.push_back(1);                                   // CID version
    bytes.push_back(static_cast<uint8_t>(content_type));  // Content-Type
    std::copy(mhash.toBuffer().begin(), mhash.toBuffer().end(),
              std::back_inserter(bytes));  // multihash data
    return bytes;
  }

  outcome::result<ContentIdentifier> ContentIdentifierCodec::decode(
      gsl::span<const uint8_t> bytes) {
    if (bytes.size() == 34 and bytes[0] == 0x12 and bytes[1] == 0x20) {
      OUTCOME_TRY(hash, Multihash::createFromBytes(bytes));
      return ContentIdentifier(ContentIdentifier::Version::V0,
                               MulticodecType::Code::DAG_PB, std::move(hash));
    }
    auto version_opt = UVarint::create(bytes);
    if (!version_opt) {
      return DecodeError::EMPTY_VERSION;
    }
    auto version = version_opt.value().toUInt64();
    if (version == 1) {
      auto version_length = UVarint::calculateSize(bytes);
      auto multicodec_opt = UVarint::create(
          bytes.subspan(static_cast<ptrdiff_t>(version_length)));
      if (!multicodec_opt) {
        return DecodeError::EMPTY_MULTICODEC;
      }
      auto multicodec_length = UVarint::calculateSize(
          bytes.subspan(static_cast<ptrdiff_t>(version_length)));
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

  outcome::result<std::string> ContentIdentifierCodec::toString(
      const ContentIdentifier &cid) {
    std::string result;
    OUTCOME_TRY(cid_bytes, encode(cid));
    switch (cid.version) {
      case ContentIdentifier::Version::V0:
        result = detail::encodeBase58(cid_bytes);
        break;
      case ContentIdentifier::Version::V1:
        result = MultibaseCodecImpl().encode(
            cid_bytes, MultibaseCodec::Encoding::BASE32_LOWER);
        break;
      default:
        return EncodeError::VERSION_UNSUPPORTED;
    }
    return result;
  }

  outcome::result<std::string> ContentIdentifierCodec::toStringOfBase(
      const ContentIdentifier &cid, MultibaseCodec::Encoding base) {
    std::string result;
    OUTCOME_TRY(cid_bytes, encode(cid));
    switch (cid.version) {
      case ContentIdentifier::Version::V0:
        if (base != MultibaseCodec::Encoding::BASE58)
          return EncodeError::INVALID_BASE_ENCODING;
        result = detail::encodeBase58(cid_bytes);
        break;
      case ContentIdentifier::Version::V1:
        result = MultibaseCodecImpl().encode(cid_bytes, base);
        break;
      default:
        return EncodeError::VERSION_UNSUPPORTED;
    }
    return result;
  }

  outcome::result<ContentIdentifier> ContentIdentifierCodec::fromString(
      const std::string &str) {
    if (str.size() < 2) {
      return DecodeError::CID_TOO_SHORT;
    }

    if (str.size() == 46 && str.substr(0, 2) == "Qm") {
      OUTCOME_TRY(hash, detail::decodeBase58(str));
      return decode(hash);
    }

    OUTCOME_TRY(bytes, MultibaseCodecImpl().decode(str));

    return decode(bytes);
  }
}  // namespace libp2p::multi
