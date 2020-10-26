/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_VALUESTORE
#define LIBP2P_PROTOCOL_KADEMLIA_VALUESTORE

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /// ValueStore is a basic Put/Get interface.
  class ValueStore {
   public:
    virtual ~ValueStore() = default;

    /// Adds @param value corresponding to given @param key.
    virtual outcome::result<void> putValue(Key key, Value value) = 0;

    /// Searches for the @return value corresponding to given @param key.
    virtual outcome::result<ValueAndTime> getValue(const Key &key) const = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_VALUESTORE
