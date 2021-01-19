/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_STORAGE
#define LIBP2P_PROTOCOL_KADEMLIA_STORAGE

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /**
   * Frontend of key-value storage
   */
  class Storage {
   public:
    virtual ~Storage() = default;

    /// Adds @param value corresponding to given @param key.
    virtual outcome::result<void> putValue(Key key, Value value) = 0;

    /// Searches for the @return value corresponding to given @param key.
    virtual outcome::result<ValueAndTime> getValue(const Key &key) const = 0;

    /// @returns true if it has value corresponding to given @param key.
    virtual bool hasValue(const Key &key) const = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_STORAGE
