/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SHA512_HPP
#define LIBP2P_SHA512_HPP

#include <string_view>

#include <gsl/span>
#include <libp2p/common/types.hpp>

namespace libp2p::crypto {
  /**
   * Take a SHA-512 hash from string
   * @param input to be hashed
   * @return hashed bytes
   */
  libp2p::common::Hash512 sha512(std::string_view input);

  /**
   * Take a SHA-512 hash from bytes
   * @param input to be hashed
   * @return hashed bytes
   */
  libp2p::common::Hash512 sha512(gsl::span<const uint8_t> input);
}  // namespace libp2p::crypto

#endif  // LIBP2P_SHA512_HPP
