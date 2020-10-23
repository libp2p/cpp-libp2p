/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREBACKEND
#define LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREBACKEND

#include <libp2p/protocol/kademlia/content_id.hpp>
#include <libp2p/protocol/kademlia/content_value.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::protocol::kademlia {

class ValueStoreBackend {
public:
		using Key = ContentId;
		using Value = ContentValue;

		virtual ~ValueStoreBackend() = default;

		/// Adds @param value corresponding to given @param key.
		virtual outcome::result<void> putValue(Key key, Value value) = 0;

		/// Searches for the @return value corresponding to given @param key.
		virtual outcome::result<Value> getValue(const Key &key) = 0;

		/// Removes value corresponded to given @param key.
    virtual outcome::result<void> erase(const Key &key) = 0;
};



}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREBACKEND
