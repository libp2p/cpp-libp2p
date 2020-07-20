/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_PROTOCOL_KADEMLIA_IMPL_DEFAULT_VALUE_STORE_HPP
#define LIBP2P_INCLUDE_LIBP2P_PROTOCOL_KADEMLIA_IMPL_DEFAULT_VALUE_STORE_HPP

#include <libp2p/protocol/kademlia/value_store_backend.hpp>
#include <unordered_map>

namespace libp2p::protocol::kademlia {

  struct DefaultValueStore : public ValueStoreBackend {
    DefaultValueStore() = default;
    ~DefaultValueStore() override = default;

    /// PutValue adds value corresponding to given Key.
    bool putValue(ContentAddress key, Value value) override;

    /// GetValue searches for the value corresponding to given Key.
    [[nodiscard]] outcome::result<Value> getValue(
        const ContentAddress &key) const override;

    void erase(const ContentAddress &key) override;

   private:
    std::unordered_map<ContentAddress, Value> values_;
  };
}  // namespace kagome::protocol::kademlia

#endif  // LIBP2P_INCLUDE_LIBP2P_PROTOCOL_KADEMLIA_IMPL_DEFAULT_VALUE_STORE_HPP
