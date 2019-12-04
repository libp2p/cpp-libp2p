/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_RESPONSE_HANDLER_HPP
#define LIBP2P_KAD_RESPONSE_HANDLER_HPP

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/impl/kad_message.hpp>
#include <memory>

namespace libp2p::protocol::kademlia {

  class KadResponseHandler
      : public std::enable_shared_from_this<KadResponseHandler> {
   public:
    using Ptr = std::shared_ptr<KadResponseHandler>;
    virtual ~KadResponseHandler() = default;

    virtual Message::Type expectedResponseType() = 0;

    virtual bool needResponse() = 0;

    virtual void onResult(const peer::PeerId &from,
                          outcome::result<Message> result) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_RESPONSE_HANDLER_HPP
