/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASE64_HPP
#define LIBP2P_BASE64_HPP

#include <optional>

#include <libp2p/common/types.hpp>
#include <libp2p/outcome/outcome.hpp>

/**
 * Encode/decode to/from base64 format
 * Implementation is taken from https://github.com/boostorg/beast, as it is
 * not safe to use the Boost's code directly - it lies in 'detail' namespace,
 * which should not be touched externally
 */
namespace libp2p::multi::detail {

  /**
   * Encode bytes to base64 string
   * @param bytes to be encoded
   * @return encoded string
   */
  std::string encodeBase64(const common::ByteArray &bytes);

  /**
   * Decode base64 string to bytes
   * @param string to be decoded
   * @return decoded bytes in case of success
   */
  outcome::result<common::ByteArray> decodeBase64(std::string_view string);
}  // namespace libp2p::multi::detail

#endif  // LIBP2P_BASE64_HPP
