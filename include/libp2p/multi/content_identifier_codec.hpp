/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONTENT_IDENTIFIER_CODEC_HPP
#define LIBP2P_CONTENT_IDENTIFIER_CODEC_HPP

#include <libp2p/multi/content_identifier.hpp>

namespace libp2p::multi {

  /**
   * Serializes and deserializes CID to byte representation.
   * To serialize it to a multibase encoded string, use MultibaseCodec
   * @see MultibaseCodec
   */
  class ContentIdentifierCodec {
   public:
    enum class EncodeError {
      INVALID_CONTENT_TYPE = 1,
      INVALID_HASH_TYPE,
      INVALID_HASH_LENGTH
    };

    enum class DecodeError {
      EMPTY_VERSION = 1,
      EMPTY_MULTICODEC,
      MALFORMED_VERSION,
      RESERVED_VERSION
    };

    static outcome::result<std::vector<uint8_t>> encode(
        const ContentIdentifier &cid);

    /// Encodes arbitrary byte buffer into CID v.0 wire format
    static std::vector<uint8_t> encodeCIDV0(
        const void* byte_buffer, size_t sz);

    static outcome::result<ContentIdentifier> decode(
        gsl::span<const uint8_t> bytes);
  };

}  // namespace libp2p::multi

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi, ContentIdentifierCodec::EncodeError);
OUTCOME_HPP_DECLARE_ERROR(libp2p::multi, ContentIdentifierCodec::DecodeError);

#endif  // LIBP2P_CONTENT_IDENTIFIER_CODEC_HPP
