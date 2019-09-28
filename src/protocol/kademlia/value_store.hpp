/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_VALUE_STORE_HPP
#define LIBP2P_KADEMLIA_VALUE_STORE_HPP

#include "protocol/kademlia/common.hpp"

namespace libp2p::protocol::kademlia {

  /// ValueStore is a basic Put/Get interface.
  struct ValueStore {
    using PutValueResult = outcome::result<void>;
    using PutValueResultFunc = std::function<void(PutValueResult)>;

    using GetValueResult = outcome::result<Value>;
    using GetValueResultFunc = std::function<void(GetValueResult)>;

    virtual ~ValueStore() = default;

    /// PutValue adds value corresponding to given Key.
    virtual void putValue(Key key, Value value, PutValueResultFunc f) = 0;

    /// GetValue searches for the value corresponding to given Key.
    virtual void getValue(Key key, GetValueResultFunc f) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KADEMLIA_VALUE_STORE_HPP
