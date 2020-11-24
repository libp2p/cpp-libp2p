/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/validator_default.hpp>

#include <libp2p/protocol/kademlia/error.hpp>

namespace libp2p::protocol::kademlia {

  outcome::result<void> ValidatorDefault::validate(const Key &key,
                                                   const Value &value) {
    return outcome::success();
  }

  outcome::result<size_t> ValidatorDefault::select(
      const Key &key, const std::vector<Value> &values) {
    if (values.empty()) {
      return Error::VALUE_NOT_FOUND;
    }
    return 0;
  }

}  // namespace libp2p::protocol::kademlia
