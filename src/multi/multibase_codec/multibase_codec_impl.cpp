/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>

#include <unordered_map>

#include <boost/optional.hpp>
#include <libp2p/multi/multibase_codec/codecs/base16.hpp>
#include <libp2p/multi/multibase_codec/codecs/base32.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <libp2p/multi/multibase_codec/codecs/base64.hpp>

namespace {
  using namespace libp2p::multi;          // NOLINT
  using namespace libp2p::multi::detail;  // NOLINT

  /**
   * Get the encoding by a character
   * @param ch of encoding
   * @return related encoding, if character stands for one of them, none
   * otherwise
   */
  boost::optional<MultibaseCodec::Encoding> encodingByChar(char ch) {
    switch (ch) {
      case 'f':
        return MultibaseCodec::Encoding::BASE16_LOWER;
      case 'F':
        return MultibaseCodec::Encoding::BASE16_UPPER;
      case 'b':
        return MultibaseCodec::Encoding::BASE32_LOWER;
      case 'B':
        return MultibaseCodec::Encoding::BASE32_UPPER;
      case 'z':
        return MultibaseCodec::Encoding::BASE58;
      case 'm':
        return MultibaseCodec::Encoding::BASE64;
      default:
        return boost::none;
    }
  }

  struct CodecFunctions {
    using EncodeFuncType = decltype(encodeBase64);
    using DecodeFuncType = decltype(decodeBase64);

    EncodeFuncType *encode;
    DecodeFuncType *decode;
  };

  /// all available codec functions
  const std::unordered_map<MultibaseCodec::Encoding, CodecFunctions> codecs{
      {MultibaseCodec::Encoding::BASE16_UPPER,
       {&encodeBase16Upper, &decodeBase16Upper}},
      {MultibaseCodec::Encoding::BASE16_LOWER,
       {&encodeBase16Lower, &decodeBase16Lower}},
      {MultibaseCodec::Encoding::BASE32_UPPER,
       {&encodeBase32Upper, &decodeBase32Upper}},
      {MultibaseCodec::Encoding::BASE32_LOWER,
       {&encodeBase32Lower, &decodeBase32Lower}},
      {MultibaseCodec::Encoding::BASE58, {&encodeBase58, &decodeBase58}},
      {MultibaseCodec::Encoding::BASE64, {&encodeBase64, &decodeBase64}}};
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, MultibaseCodecImpl::Error, e) {
  using E = libp2p::multi::MultibaseCodecImpl::Error;
  switch (e) {
    case E::INPUT_TOO_SHORT:
      return "Input must be at least two bytes long";
    case E::UNSUPPORTED_BASE:
      return "The base is either not supported or does not exist";
    default:
      return "Unknown error";
  }
}

namespace libp2p::multi {
  using common::ByteArray;

  std::string MultibaseCodecImpl::encode(const ByteArray &bytes,
                                         Encoding encoding) const {
    if (bytes.empty()) {
      return "";
    }

    return static_cast<char>(encoding) + codecs.at(encoding).encode(bytes);
  }

  outcome::result<ByteArray> MultibaseCodecImpl::decode(
      std::string_view string) const {
    if (string.length() < 2) {
      return Error::INPUT_TOO_SHORT;
    }

    auto encoding_base = encodingByChar(string.front());
    if (!encoding_base) {
      return Error::UNSUPPORTED_BASE;
    }

    return codecs.at(*encoding_base).decode(string.substr(1));
  }
}  // namespace libp2p::multi
