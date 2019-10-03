/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SHA256_HPP
#define LIBP2P_SHA256_HPP

#include <string_view>

#include <gsl/span>
#include <libp2p/common/types.hpp>

namespace libp2p::crypto {
  /**
   * Take a SHA-256 hash from string
   * @param input to be hashed
   * @return hashed bytes
   */
  libp2p::common::Hash256 sha256(std::string_view input);

  /**
   * Take a SHA-256 hash from bytes
   * @param input to be hashed
   * @return hashed bytes
   */
  libp2p::common::Hash256 sha256(gsl::span<const uint8_t> input);
}  // namespace libp2p::crypto

#endif  // LIBP2P_SHA256_HPP
