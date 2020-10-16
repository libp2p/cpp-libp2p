/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SHA512_HPP
#define LIBP2P_SHA512_HPP

#include <string_view>

#include <openssl/sha.h>
#include <gsl/span>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/hasher.hpp>

namespace libp2p::crypto {

  class Sha512 : public Hasher {
   public:
    Sha512();

    ~Sha512() override;

    outcome::result<void> write(gsl::span<const uint8_t> data) override;

    outcome::result<std::vector<uint8_t>> digest() override;

    outcome::result<void> reset() override;

    size_t digestSize() const override;

    size_t blockSize() const override;

   private:
    void sinkCtx();

    SHA512_CTX ctx_;
    bool initialized_;
  };

  /**
   * Take a SHA-512 hash from string
   * @param input to be hashed
   * @return hashed bytes
   */
  [[deprecated]] libp2p::common::Hash512 sha512(std::string_view input);

  /**
   * Take a SHA-512 hash from bytes
   * @param input to be hashed
   * @return hashed bytes
   */
  [[deprecated]] libp2p::common::Hash512 sha512(gsl::span<const uint8_t> input);
}  // namespace libp2p::crypto

#endif  // LIBP2P_SHA512_HPP
