/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/crypto/noise_sha256.hpp>

namespace libp2p::security::noise {

  std::string NoiseSHA256HasherImpl::hashName() const {
    return "SHA256";
  }

  std::shared_ptr<libp2p::crypto::Hasher> NoiseSHA256HasherImpl::hash() {
    return std::make_shared<libp2p::crypto::Sha256>();
  }

}  // namespace libp2p::security::noise
