/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

#include <boost/optional.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/common_functions.hpp>
#include <libp2p/crypto/hasher.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::security::noise {

  using Key32 = std::array<uint8_t, 32>;
  using libp2p::crypto::asArray;
  using libp2p::crypto::asVector;

  using crypto::common::HashType;
  using libp2p::Bytes;
  struct HKDFResult {
    Bytes one;
    Bytes two;
    Bytes three;
  };

  enum class HKDFError {
    ILLEGAL_OUTPUTS_NUMBER = 1,
  };

  template <typename T>
  std::vector<T> spanToVec(std::span<const T> data) {
    return std::vector<T>(data.begin(), data.end());
  }

  outcome::result<HKDFResult> hkdf(HashType hash_type,
                                   size_t outputs,
                                   BytesIn chaining_key,
                                   BytesIn input_key_material);

  struct DHKey {
    Bytes priv;
    Bytes pub;
  };

  class DiffieHellman {
   public:
    virtual ~DiffieHellman() = default;

    /// generates a key pair
    virtual outcome::result<DHKey> generate() = 0;

    /// does a Diffie-Hellman calculation between the given keys
    virtual outcome::result<Bytes> dh(const Bytes &private_key,
                                      const Bytes &public_key) = 0;

    /// returns the size in bytes of the result of dh computation
    virtual int dhSize() const = 0;

    /// algorithm identifier used in Noise handshake
    virtual std::string dhName() const = 0;
  };

  class NamedHasher {
   public:
    virtual ~NamedHasher() = default;

    virtual std::shared_ptr<crypto::Hasher> hash() = 0;

    virtual std::string hashName() const = 0;
  };

  // has to be initialized with a key
  class AEADCipher {
   public:
    virtual ~AEADCipher() = default;

    virtual outcome::result<Bytes> encrypt(BytesIn precompiled_out,
                                           uint64_t nonce,
                                           BytesIn plaintext,
                                           BytesIn aad) = 0;

    virtual outcome::result<Bytes> decrypt(BytesIn precompiled_out,
                                           uint64_t nonce,
                                           BytesIn ciphertext,
                                           BytesIn aad) = 0;
  };

  class NamedAEADCipher {
   public:
    virtual ~NamedAEADCipher() = default;

    virtual std::shared_ptr<AEADCipher> cipher(Key32 key) = 0;

    virtual std::string cipherName() const = 0;
  };

  /// A set of three algos: DH, hash and AEAD cipher
  class CipherSuite : public DiffieHellman,
                      public NamedHasher,
                      public NamedAEADCipher {
   public:
    virtual ~CipherSuite() = default;

    virtual std::string name() = 0;
  };

}  // namespace libp2p::security::noise

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::noise, HKDFError);
