/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KEY_REPOSITORY_MOCK_HPP
#define LIBP2P_KEY_REPOSITORY_MOCK_HPP

#include <libp2p/peer/key_repository.hpp>

#include <gmock/gmock.h>

namespace libp2p::peer {
  struct KeyRepositoryMock : public KeyRepository {
    ~KeyRepositoryMock() override = default;

    MOCK_METHOD1(clear, void(const PeerId &));

    MOCK_METHOD1(getPublicKeys, outcome::result<PubVecPtr>(const PeerId &));

    MOCK_METHOD2(addPublicKey,
                 outcome::result<void>(const PeerId &,
                                       const crypto::PublicKey &));

    MOCK_METHOD0(getKeyPairs, outcome::result<KeyPairVecPtr>());

    MOCK_METHOD1(addKeyPair, outcome::result<void>(const KeyPair &));

    MOCK_CONST_METHOD0(getPeers, std::unordered_set<PeerId>());
  };
}  // namespace libp2p::peer

#endif  // LIBP2P_KEY_REPOSITORY_MOCK_HPP
