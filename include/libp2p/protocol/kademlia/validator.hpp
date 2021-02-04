/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_VALIDATOR
#define LIBP2P_PROTOCOL_KADEMLIA_VALIDATOR

#include <vector>

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  class Validator {
   public:
    virtual ~Validator() = default;

    /**
     * Validates the given record.
     * Calling to validate:
     *  - incoming values in response to GET_VALUE calls
     *  - outgoing values before storing them in the network via PUT_VALUE calls
     * @returns an error if it's invalid (e.g., expired, signed by the wrong
     * key, etc.)
     */
    virtual outcome::result<void> validate(const Key &key,
                                           const Value &value) = 0;

    /**
     * Selects the best record from the set of records (by time, order or other
     * heuristic)
     * @note Decisions made by select should be stable
     * @returns index of the best record
     */
    virtual outcome::result<size_t> select(
        const Key &key, const std::vector<Value> &values) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_VALIDATOR
