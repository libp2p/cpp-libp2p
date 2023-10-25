/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/protocol_repository.hpp>

#include <gmock/gmock.h>

namespace libp2p::peer {
  struct ProtocolRepositoryMock : public ProtocolRepository {
    MOCK_METHOD2(addProtocols,
                 outcome::result<void>(const PeerId &,
                                       std::span<const ProtocolName>));

    MOCK_METHOD2(removeProtocols,
                 outcome::result<void>(const PeerId &,
                                       std::span<const ProtocolName>));

    MOCK_CONST_METHOD1(
        getProtocols,
        outcome::result<std::vector<ProtocolName>>(const PeerId &));

    MOCK_CONST_METHOD2(supportsProtocols,
                       outcome::result<std::vector<ProtocolName>>(
                           const PeerId &, const std::set<ProtocolName> &));

    MOCK_CONST_METHOD2(addProtocols,
                       outcome::result<std::vector<ProtocolName>>(
                           const PeerId &, const std::set<ProtocolName> &));

    MOCK_METHOD1(clear, void(const PeerId &));

    MOCK_CONST_METHOD0(getPeers, std::unordered_set<PeerId>());

    MOCK_METHOD0(collectGarbage, void());
  };
}  // namespace libp2p::peer
