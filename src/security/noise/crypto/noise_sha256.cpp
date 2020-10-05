/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SRC_SECURITY_NOISE_CRYPTO_NOISE_SHA256_CPP
#define LIBP2P_SRC_SECURITY_NOISE_CRYPTO_NOISE_SHA256_CPP

#include <libp2p/security/noise/crypto/noise_sha256.hpp>

namespace libp2p::security::noise {

  std::string SHA256::hashName() const {
    return "SHA256";
  }

  std::shared_ptr<libp2p::crypto::Hash> SHA256::hash() {
    return std::make_shared<libp2p::crypto::Sha256>();
  }

}  // namespace libp2p::security::noise

#endif  // LIBP2P_SRC_SECURITY_NOISE_CRYPTO_NOISE_SHA256_CPP
