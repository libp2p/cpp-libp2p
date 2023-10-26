/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <span>

namespace libp2p::crypto {

  using libp2p::crypto::common::HashType;

  class Hasher {
   public:
    virtual ~Hasher() = default;

    /// appends a new chunk of data
    virtual outcome::result<void> write(BytesIn data) = 0;

    /**
     * Calculates the current digest.
     * Does not affect the internal state.
     * New data still could be fed via write method.
     */
    virtual outcome::result<void> digestOut(BytesOut out) const = 0;

    /// resets the internal state
    virtual outcome::result<void> reset() = 0;

    /// hash size in bytes
    virtual size_t digestSize() const = 0;

    /// block size in bytes for the most optimal hash update via write method
    virtual size_t blockSize() const = 0;

    /// runtime identifiable hasher type
    virtual HashType hashType() const = 0;

    outcome::result<libp2p::Bytes> digest() const {
      outcome::result<libp2p::Bytes> result{outcome::success()};
      result.value().resize(digestSize());
      OUTCOME_TRY(digestOut(result.value()));
      return result;
    }
  };
}  // namespace libp2p::crypto
