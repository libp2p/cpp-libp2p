/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_IDENTITY_MANAGER_MOCK_HPP
#define LIBP2P_IDENTITY_MANAGER_MOCK_HPP

#include <libp2p/peer/identity_manager.hpp>

#include <gmock/gmock.h>

namespace libp2p::peer {

  struct IdentityManagerMock : public IdentityManager {
    ~IdentityManagerMock() override = default;

    MOCK_CONST_METHOD0(getId, const peer::PeerId &());
    MOCK_CONST_METHOD0(getKeyPair, const crypto::KeyPair &());
  };

}  // namespace libp2p::peer

#endif  // LIBP2P_IDENTITY_MANAGER_MOCK_HPP
