/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  class NoiseSHA256HasherImpl : public NamedHasher {
   public:
    std::shared_ptr<libp2p::crypto::Hasher> hash() override;

    std::string hashName() const override;
  };

}  // namespace libp2p::security::noise
