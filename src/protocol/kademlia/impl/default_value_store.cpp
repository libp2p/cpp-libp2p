/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/default_value_store.hpp>

namespace libp2p::protocol::kademlia {

  bool DefaultValueStore::putValue(ContentAddress key, Value value) {
    values_[std::move(key)] = std::move(value);
    return true;  // N.B. Real backend should validate key and value
  }

  outcome::result<Value> DefaultValueStore::getValue(
      const ContentAddress &key) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
      return Error::VALUE_NOT_FOUND;
    }
    return it->second;
  }

  void DefaultValueStore::erase(const ContentAddress &key) {
    values_.erase(key);
  }

}  // namespace libp2p::protocol::kademlia
