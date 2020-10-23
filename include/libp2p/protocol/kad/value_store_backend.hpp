/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_VALUE_STORE_BACKEND_HPP
#define LIBP2P_KADEMLIA_VALUE_STORE_BACKEND_HPP

#include <libp2p/protocol/kad/common.hpp>

namespace libp2p::protocol::kad {

  /// ValueStore is a backend for Kademlia key/value storage
  struct ValueStoreBackend {
    virtual ~ValueStoreBackend() = default;

    /// PutValue validates key and value and, on success, stores it
    virtual outcome::result<void> putValue(ContentAddress key, Value value) = 0;

    /// GetValue searches for the value corresponding to given Key.
    virtual outcome::result<Value> getValue(const ContentAddress& key) const = 0;

    virtual outcome::result<void> erase(const ContentAddress& key) = 0;
  };

}  // namespace libp2p::protocol::kad

#endif  // LIBP2P_KADEMLIA_VALUE_STORE_BACKEND_HPP
