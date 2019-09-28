/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/impl/router_impl.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::network, RouterImpl::Error, e) {
  using E = libp2p::network::RouterImpl::Error;
  switch (e) {
    case E::NO_HANDLER_FOUND:
      return "no handler was found for a given protocol";
  }
  return "unknown error";
}

namespace libp2p::network {
  void RouterImpl::setProtocolHandler(const peer::Protocol &protocol,
                                      const ProtoHandler &handler) {
    setProtocolHandler(protocol, handler, [protocol](const auto &p) {
      // perfect match func
      return protocol == p;
    });
  }

  void RouterImpl::setProtocolHandler(const peer::Protocol &protocol,
                                      const ProtoHandler &handler,
                                      const ProtoPredicate &predicate) {
    proto_handlers_[protocol] = PredicateAndHandler{predicate, handler};
  }

  std::vector<peer::Protocol> RouterImpl::getSupportedProtocols() const {
    std::vector<peer::Protocol> protos;
    if (proto_handlers_.empty()) {
      return protos;
    }

    std::string key_buffer;  // a workaround, recommended by the library's devs
    protos.reserve(proto_handlers_.size());
    for (auto it = proto_handlers_.begin(); it != proto_handlers_.end(); ++it) {
      it.key(key_buffer);
      protos.push_back(std::move(key_buffer));
    }
    return protos;
  }

  void RouterImpl::removeProtocolHandlers(const peer::Protocol &protocol) {
    proto_handlers_.erase_prefix(protocol);
  }

  void RouterImpl::removeAll() {
    proto_handlers_.clear();
  }

  outcome::result<void> RouterImpl::handle(
      const peer::Protocol &p, std::shared_ptr<connection::Stream> stream) {
    // firstly, try to find the longest prefix - even if it's not perfect match,
    // but a predicate one, it still will save the resources
    auto matched_proto = proto_handlers_.longest_prefix(p);
    if (matched_proto == proto_handlers_.end()) {
      return Error::NO_HANDLER_FOUND;
    }

    const auto &pred_hand = matched_proto.value();
    if (matched_proto.key() == p || pred_hand.predicate(p)) {
      // perfect or predicate match
      pred_hand.handler(std::move(stream));
      return outcome::success();
    }

    // fallback: find all matches for the first two letters of the given (the
    // first letter is a '/', so we need two) protocol and test them against the
    // predicate; the longest match is to be called
    auto matched_protos = proto_handlers_.equal_prefix_range_ks(p.data(), 2);

    std::reference_wrapper<const ProtoHandler> longest_match{
        matched_protos.first.value().handler};
    size_t longest_match_size = 0;
    for (auto match = matched_protos.first; match != matched_protos.second;
         ++match) {
      if (match.value().predicate(p)
          && match.key().size() > longest_match_size) {
        longest_match_size = match.key().size();
        longest_match = match.value().handler;
      }
    }

    if (longest_match_size == 0) {
      return Error::NO_HANDLER_FOUND;
    }
    longest_match(std::move(stream));
    return outcome::success();
  }

}  // namespace libp2p::network
