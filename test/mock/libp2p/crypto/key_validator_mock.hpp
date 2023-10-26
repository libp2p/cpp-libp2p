/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/key_validator.hpp>

#include <gmock/gmock.h>

namespace libp2p::crypto::validator {

  class KeyValidatorMock : public KeyValidator {
   public:
    ~KeyValidatorMock() override = default;

    MOCK_CONST_METHOD1(validate, outcome::result<void>(const PrivateKey &));

    MOCK_CONST_METHOD1(validate, outcome::result<void>(const PublicKey &));

    MOCK_CONST_METHOD1(validate, outcome::result<void>(const KeyPair &));
  };
}  // namespace libp2p::crypto::validator
