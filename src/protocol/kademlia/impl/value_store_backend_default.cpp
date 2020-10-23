/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/value_store_backend_default.hpp>

namespace libp2p::protocol::kademlia {

  outcome::result<void> ValueStoreBackendDefault::putValue(
      ValueStoreBackend::Key key, ValueStoreBackend::Value value) {
    values_.emplace(std::move(key), std::move(value));
    return outcome::success();
  }

  outcome::result<ValueStoreBackend::Value> ValueStoreBackendDefault::getValue(
      const ValueStoreBackend::Key &key) {
    if (auto it = values_.find(key); it != values_.end()) {
      return it->second;
    }
    return Error::VALUE_NOT_FOUND;
  }

  outcome::result<void> ValueStoreBackendDefault::erase(
      const ValueStoreBackend::Key &key) {
    values_.erase(key);
    return outcome::success();
  }

}  // namespace libp2p::protocol::kademlia

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol::kademlia,
                            ValueStoreBackendDefault::Error, e) {
  using E = libp2p::protocol::kademlia::ValueStoreBackendDefault::Error;
  switch (e) {
    case E::VALUE_NOT_FOUND:
      return "value not found";
  }
  return "unknown ValueStoreBackendDefault::Error";
}
