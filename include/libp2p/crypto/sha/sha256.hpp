/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SHA256_HPP
#define LIBP2P_SHA256_HPP

#include <openssl/sha.h>
#include <gsl/span>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/hasher.hpp>

namespace libp2p::crypto {

  class Sha256 : public Hasher {
   public:
    Sha256();

    ~Sha256() override;

    outcome::result<void> write(gsl::span<const uint8_t> data) override;

    outcome::result<void> digestOut(gsl::span<uint8_t> out) const override;

    outcome::result<void> reset() override;

    size_t digestSize() const override;

    size_t blockSize() const override;

    HashType hashType() const override;

   private:
    void sinkCtx();

    SHA256_CTX ctx_;
    bool initialized_;
  };

  /**
   * Take a SHA-256 hash from bytes
   * @param input to be hashed
   * @return hashed bytes
   */
  outcome::result<libp2p::common::Hash256> sha256(
      gsl::span<const uint8_t> input);
}  // namespace libp2p::crypto

#endif  // LIBP2P_SHA256_HPP
