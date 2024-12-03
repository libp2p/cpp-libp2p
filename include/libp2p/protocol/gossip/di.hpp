/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/gossip/gossip.hpp>

namespace libp2p::protocol::gossip {
  class Gossip;

  struct GossipDi {
    GossipDi(std::shared_ptr<basic::Scheduler> scheduler,
             std::shared_ptr<Host> host,
             std::shared_ptr<peer::IdentityManager> id_mgr,
             std::shared_ptr<crypto::CryptoProvider> crypto_provider,
             std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
             Config config)
        : impl{create(std::move(scheduler),
                      std::move(host),
                      std::move(id_mgr),
                      std::move(crypto_provider),
                      std::move(key_marshaller),
                      std::move(config))} {}

    std::shared_ptr<Gossip> impl;
  };
}  // namespace libp2p::protocol::gossip
