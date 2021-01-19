/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_STORAGEBACKEND
#define LIBP2P_PROTOCOL_KADEMLIA_STORAGEBACKEND

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /**
   * Backend of key-value storage
   */
  class StorageBackend {
   public:
    virtual ~StorageBackend() = default;

    /// Adds @param value corresponding to given @param key.
    virtual outcome::result<void> putValue(Key key, Value value) = 0;

    /// Searches for the @return value corresponding to given @param key.
    virtual outcome::result<Value> getValue(const Key &key) const = 0;

    /// Removes value corresponded to given @param key.
    virtual outcome::result<void> erase(const Key &key) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_STORAGEBACKEND
