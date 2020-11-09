/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_VALIDATORDEFAULT
#define LIBP2P_PROTOCOL_KADEMLIA_VALIDATORDEFAULT

#include <libp2p/protocol/kademlia/validator.hpp>

namespace libp2p::protocol::kademlia {

  class ValidatorDefault : public Validator {
   public:
    ~ValidatorDefault() override = default;

    /// @see Validator::validator
    outcome::result<void> validate(const Key &key, const Value &value) override;

    /// @see Validator::select
    outcome::result<size_t> select(const Key &key,
                                   const std::vector<Value> &values) override;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_VALIDATORDEFAULT
