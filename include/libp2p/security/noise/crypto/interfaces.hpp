/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_INTERFACES_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_INTERFACES_HPP

#include <tuple>

#include <boost/optional.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/hash.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::security::noise {

  using crypto::common::HashType;
  using libp2p::common::ByteArray;
  struct HKDFResult {
    ByteArray one;
    ByteArray two;
    ByteArray three;
  };

  enum class HKDFError {
    ILLEGAL_OUTPUTS_NUMBER = 1,
  };

  outcome::result<HKDFResult> hkdf(HashType hash_type, size_t outputs,
                                   const ByteArray &chaining_key,
                                   const ByteArray &input_key_material);

  struct DHKey {
    ByteArray priv;
    ByteArray pub;
  };

  class DiffieHellman {
   public:
    virtual ~DiffieHellman() = default;

    /// generates a key pair
    virtual outcome::result<DHKey> generate() = 0;

    /// does a Diffie-Hellman calculation between the given keys
    virtual outcome::result<ByteArray> dh(const ByteArray &private_key,
                                          const ByteArray &public_key) = 0;

    /// returns the size in bytes of the result of dh computation
    virtual int dhSize() const = 0;

    /// algorithm identifier used in Noise handshake
    virtual std::string dhName() const = 0;
  };

}  // namespace libp2p::security::noise

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::noise, HKDFError);

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_INTERFACES_HPP
