/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::aes {

  /**
   * An interface for stream data cipher encryption or decryption engine with
   * AES128 or AES256 algorithms.
   */
  class AesCtr {
   public:
    virtual ~AesCtr() = default;

    /**
     * Encrypts or decrypts user data
     * @param data to be processed
     * @return processed data chunk or an error
     */
    virtual outcome::result<Bytes> crypt(BytesIn data) const = 0;

    /**
     * Does stream data finalization
     * @return bytes buffer to correctly pad all the previously processed
     * data or an error
     */
    virtual outcome::result<Bytes> finalize() = 0;
  };
}  // namespace libp2p::crypto::aes
