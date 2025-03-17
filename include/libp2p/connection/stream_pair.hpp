/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

namespace libp2p::basic {
  class Scheduler;
}  // namespace libp2p::basic

namespace libp2p::connection {
  struct Stream;

  std::pair<std::shared_ptr<Stream>, std::shared_ptr<Stream>> streamPair(
      std::shared_ptr<basic::Scheduler> post, PeerId peer1, PeerId peer2);
}  // namespace libp2p::connection
