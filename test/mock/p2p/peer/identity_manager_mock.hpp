/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IDENTITY_MANAGER_MOCK_HPP
#define KAGOME_IDENTITY_MANAGER_MOCK_HPP

#include <gmock/gmock.h>
#include "p2p/peer/identity_manager.hpp"

namespace libp2p::peer {

  struct IdentityManagerMock : public IdentityManager {
    ~IdentityManagerMock() override = default;

    MOCK_CONST_METHOD0(getId, const peer::PeerId &());
    MOCK_CONST_METHOD0(getKeyPair, const crypto::KeyPair &());
  };

}  // namespace libp2p::peer

#endif  // KAGOME_IDENTITY_MANAGER_MOCK_HPP
