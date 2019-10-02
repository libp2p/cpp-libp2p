/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASE16_HPP
#define LIBP2P_BASE16_HPP

#include <optional>

#include <libp2p/common/types.hpp>
#include <libp2p/outcome/outcome.hpp>

/**
 * Encode/decode to/from base16 format
 */
namespace libp2p::multi::detail {

  /**
   * Encode bytes to base16 uppercase string
   * @param bytes to be encoded
   * @return encoded string
   */
  std::string encodeBase16Upper(const common::ByteArray &bytes);
  /**
   * Encode bytes to base16 lowercase string
   * @param bytes to be encoded
   * @return encoded string
   */
  std::string encodeBase16Lower(const common::ByteArray &bytes);

  /**
   * Decode base16 uppercase to bytes
   * @param string to be decoded
   * @return decoded bytes in case of success
   */
  outcome::result<common::ByteArray> decodeBase16Upper(std::string_view string);
  /**
   * Decode base16 lowercase string to bytes
   * @param string to be decoded
   * @return decoded bytes in case of success
   */
  outcome::result<common::ByteArray> decodeBase16Lower(std::string_view string);
}  // namespace libp2p::multi::detail

#endif  // LIBP2P_BASE16_HPP
