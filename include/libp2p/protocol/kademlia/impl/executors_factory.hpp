/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_EXECUTORSFACTORY
#define LIBP2P_PROTOCOL_KADEMLIA_EXECUTORSFACTORY

#include <memory>
#include <queue>
#include <unordered_set>

#include <libp2p/common/types.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/peer_id_with_distance.hpp>
#include <libp2p/protocol/kademlia/impl/response_handler.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>
#include <libp2p/protocol/kademlia/peer_routing.hpp>

namespace libp2p::protocol::kademlia {

  class PutValueExecutor;
  class GetValueExecutor;
  class AddProviderExecutor;
  class FindProvidersExecutor;
  class FindPeerExecutor;

  class ExecutorsFactory {
   public:
    virtual ~ExecutorsFactory() = default;

    virtual std::shared_ptr<PutValueExecutor> createPutValueExecutor(
        ContentId key, ContentValue value, std::vector<PeerId> addressees) = 0;

    virtual std::shared_ptr<GetValueExecutor> createGetValueExecutor(
        ContentId sought_key, FoundValueHandler handler) = 0;

    virtual std::shared_ptr<AddProviderExecutor> createAddProviderExecutor(
        ContentId key) = 0;

    virtual std::shared_ptr<FindProvidersExecutor> createGetProvidersExecutor(
        ContentId sought_key, FoundProvidersHandler handler) = 0;

    virtual std::shared_ptr<FindPeerExecutor> createFindPeerExecutor(
        PeerId peer_id, FoundPeerInfoHandler handler) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_EXECUTORSFACTORY
