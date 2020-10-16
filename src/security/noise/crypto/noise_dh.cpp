/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/error.hpp>
#include <libp2p/security/noise/crypto/noise_dh.hpp>

namespace libp2p::security::noise {

  outcome::result<DHKey> NoiseDiffieHellmanImpl::generate() {
    OUTCOME_TRY(keypair, x25519.generate());
    ByteArray priv{keypair.private_key.begin(), keypair.private_key.end()};
    ByteArray pub{keypair.public_key.begin(), keypair.public_key.end()};
    return DHKey{.priv = std::move(priv), .pub = std::move(pub)};
  }

  outcome::result<ByteArray> NoiseDiffieHellmanImpl::dh(
      const ByteArray &private_key, const ByteArray &public_key) {
    crypto::x25519::PrivateKey priv;
    crypto::x25519::PublicKey pub;
    if (private_key.size() != priv.size() or public_key.size() != pub.size()) {
      return crypto::OpenSslError::WRONG_KEY_SIZE;
    }
    std::copy_n(private_key.begin(), priv.size(), priv.begin());
    std::copy_n(public_key.begin(), pub.size(), pub.begin());
    return x25519.dh(priv, pub);
  }

  int NoiseDiffieHellmanImpl::dhSize() const {
    return 32;
  }

  std::string NoiseDiffieHellmanImpl::dhName() const {
    return "25519";
  }
}  // namespace libp2p::security::noise
