/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_REPOSITORY_MOCK_HPP
#define LIBP2P_PROTOCOL_REPOSITORY_MOCK_HPP

#include <libp2p/peer/protocol_repository.hpp>

#include <gmock/gmock.h>

namespace libp2p::peer {
  struct ProtocolRepositoryMock : public ProtocolRepository {
    MOCK_METHOD2(addProtocols,
                 outcome::result<void>(const PeerId &,
                                       gsl::span<const Protocol>));

    MOCK_METHOD2(removeProtocols,
                 outcome::result<void>(const PeerId &,
                                       gsl::span<const Protocol>));

    MOCK_CONST_METHOD1(getProtocols,
                       outcome::result<std::vector<Protocol>>(const PeerId &));

    MOCK_CONST_METHOD2(supportsProtocols,
                       outcome::result<std::vector<Protocol>>(
                           const PeerId &, const std::set<Protocol> &));

    MOCK_CONST_METHOD2(addProtocols,
                       outcome::result<std::vector<Protocol>>(
                           const PeerId &, const std::set<Protocol> &));

    MOCK_METHOD1(clear, void(const PeerId &));

    MOCK_CONST_METHOD0(getPeers, std::unordered_set<PeerId>());

    MOCK_METHOD0(collectGarbage, void());
  };
}  // namespace libp2p::peer

#endif  // LIBP2P_PROTOCOL_REPOSITORY_MOCK_HPP
