/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_STORAGEBACKENDDEFAULT
#define LIBP2P_PROTOCOL_KADEMLIA_STORAGEBACKENDDEFAULT

#include <libp2p/protocol/kademlia/storage_backend.hpp>

namespace libp2p::protocol::kademlia {

  class StorageBackendDefault : public StorageBackend {
   public:
    StorageBackendDefault() = default;
    ~StorageBackendDefault() override = default;

    outcome::result<void> putValue(Key key, Value value) override;

    outcome::result<Value> getValue(const Key &key) const override;

    outcome::result<void> erase(const Key &key) override;

   private:
    std::unordered_map<Key, Value> values_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_STORAGEBACKENDDEFAULT
