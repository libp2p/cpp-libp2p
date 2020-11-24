/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/storage_backend_default.hpp>

#include <libp2p/protocol/kademlia/error.hpp>

namespace libp2p::protocol::kademlia {

  outcome::result<void> StorageBackendDefault::putValue(Key key, Value value) {
    values_.emplace(std::move(key), std::move(value));
    return outcome::success();
  }

  outcome::result<Value> StorageBackendDefault::getValue(const Key &key) const {
    if (auto it = values_.find(key); it != values_.end()) {
      return it->second;
    }
    return Error::VALUE_NOT_FOUND;
  }

  outcome::result<void> StorageBackendDefault::erase(const Key &key) {
    values_.erase(key);
    return outcome::success();
  }

}  // namespace libp2p::protocol::kademlia
