/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREBACKENDDEFAULT
#define LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREBACKENDDEFAULT

#include <libp2p/protocol/kademlia/value_store_backend.hpp>

namespace libp2p::protocol::kademlia {

class ValueStoreBackendDefault : public ValueStoreBackend {
public:
		enum class Error {
				VALUE_NOT_FOUND = 1
		};

		ValueStoreBackendDefault() = default;
		~ValueStoreBackendDefault() override = default;

		outcome::result<void> putValue(Key key, Value value) override;

		outcome::result<Value> getValue(const Key &key) override;

		outcome::result<void> erase(const Key &key) override;

private:
		std::unordered_map<Key, Value> values_;

};

}  // namespace libp2p::protocol::kademlia

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, ValueStoreBackendDefault::Error)

#endif  // LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREBACKENDDEFAULT
