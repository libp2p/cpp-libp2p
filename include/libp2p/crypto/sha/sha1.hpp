/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_SHA1
#define LIBP2P_CRYPTO_SHA1

#include <openssl/sha.h>
#include <span>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/hasher.hpp>

namespace libp2p::crypto {

  class Sha1 : public Hasher {
   public:
    Sha1();

    ~Sha1() override;

    outcome::result<void> write(ConstSpanOfBytes data) override;

    outcome::result<void> digestOut(MutSpanOfBytes out) const override;

    outcome::result<void> reset() override;

    size_t digestSize() const override;

    size_t blockSize() const override;

    HashType hashType() const override;

   private:
    void sinkCtx();

    SHA_CTX ctx_;
    bool initialized_;
  };

  /**
   * Take a SHA-1 hash from bytes
   * @param input to be hashed
   * @return hashed bytes
   */
  outcome::result<libp2p::common::Hash160> sha1(ConstSpanOfBytes input);

}  // namespace libp2p::crypto

#endif  // LIBP2P_CRYPTO_SHA1
