/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multibase_codec/codecs/base_error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi::detail, BaseError, e) {
  using E = libp2p::multi::detail::BaseError;
  switch (e) {
    case E::INVALID_BASE64_INPUT:
      return "Input is not a valid base64 string";
    case E::INVALID_BASE58_INPUT:
      return "Input is not a valid base58 string";
    case E::INVALID_BASE32_INPUT:
      return "Input is not a valid base32 string";
    case E::NON_UPPERCASE_INPUT:
      return "Input is not in the uppercase hex";
    case E::NON_LOWERCASE_INPUT:
      return "Input is not in the lowercase hex";
    default:
      return "Unknown error";
  }
}
