/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /**
   * Basic Put/Get interface of Kademlia
   */
  class ValueStore {
   public:
    virtual ~ValueStore() = default;

    /// Adds @param value corresponding to given @param key.
    virtual outcome::result<void> putValue(Key key, Value value) = 0;

    /// Searches for the @return value corresponding to given @param key.
    virtual outcome::result<void> getValue(const Key &key,
                                           FoundValueHandler handler) = 0;
  };

}  // namespace libp2p::protocol::kademlia
