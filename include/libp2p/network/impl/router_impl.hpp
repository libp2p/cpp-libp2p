/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ROUTER_IMPL_HPP
#define LIBP2P_ROUTER_IMPL_HPP

#include <tsl/htrie_map.h>
#include <libp2p/network/router.hpp>

namespace libp2p::network {

  class RouterImpl : public Router {
   public:
    ~RouterImpl() override = default;

    void setProtocolHandler(StreamProtocols protocols, StreamAndProtocolCb cb,
                            ProtocolPredicate predicate = {}) override;

    std::vector<peer::ProtocolName> getSupportedProtocols() const override;

    void removeProtocolHandlers(const peer::ProtocolName &protocol) override;

    void removeAll() override;

    enum class Error { NO_HANDLER_FOUND = 1 };

    outcome::result<void> handle(
        const peer::ProtocolName &p,
        std::shared_ptr<connection::Stream> stream) override;

   private:
    struct PredicateAndHandler {
      ProtocolPredicate predicate;
      StreamAndProtocolCb handler;
    };
    tsl::htrie_map<char, PredicateAndHandler> proto_handlers_;
  };

}  // namespace libp2p::network

OUTCOME_HPP_DECLARE_ERROR(libp2p::network, RouterImpl::Error)

#endif  // LIBP2P_ROUTER_IMPL_HPP
