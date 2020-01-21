/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GOSSIP_EXAMPLE_FACTORY_HPP
#define LIBP2P_GOSSIP_EXAMPLE_FACTORY_HPP

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>

namespace libp2p::protocol::gossip::example {

  /// Creates unique host and gossip instances. Allows to make multiple
  /// instances in a process (needed for this example), scheduler and io are
  /// reused between all gossip and host instances
  std::pair<std::shared_ptr<Host>, std::shared_ptr<Gossip>> createHostAndGossip(
      Config config, std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<boost::asio::io_context> io,
      boost::optional<crypto::KeyPair> keypair = boost::none);

}  // namespace libp2p::protocol::gossip::example

#endif  // LIBP2P_GOSSIP_EXAMPLE_FACTORY_HPP
