/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/crypto_provider.hpp>

#include <gmock/gmock.h>

namespace libp2p::crypto {
  class CryptoProviderMock : public CryptoProvider {
   public:
    MOCK_METHOD(outcome::result<KeyPair>,
                generateKeys,
                (Key::Type, common::RSAKeyType),
                (const, override));

    MOCK_METHOD(outcome::result<PublicKey>,
                derivePublicKey,
                (const PrivateKey &),
                (const, override));

    MOCK_METHOD(outcome::result<Buffer>,
                sign,
                (BytesIn, const PrivateKey &),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                verify,
                (BytesIn, BytesIn, const PublicKey &),
                (const, override));
    MOCK_METHOD(outcome::result<EphemeralKeyPair>,
                generateEphemeralKeyPair,
                (common::CurveType),
                (const, override));

    MOCK_METHOD((outcome::result<std::pair<StretchedKey, StretchedKey>>),
                stretchKey,
                (common::CipherType, common::HashType, const Buffer &),
                (const, override));
  };
}  // namespace libp2p::crypto
