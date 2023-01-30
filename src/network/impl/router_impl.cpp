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
  void RouterImpl::setProtocolHandler(StreamProtocols protocols,
                                      StreamAndProtocolCb cb,
                                      ProtocolPredicate predicate) {
    for (auto &protocol : protocols) {
      proto_handlers_[protocol] = PredicateAndHandler{predicate, cb};
    }
  }

  std::vector<peer::ProtocolName> RouterImpl::getSupportedProtocols() const {
    std::vector<peer::ProtocolName> protos;
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

  void RouterImpl::removeProtocolHandlers(const peer::ProtocolName &protocol) {
    proto_handlers_.erase_prefix(protocol);
  }

  void RouterImpl::removeAll() {
    proto_handlers_.clear();
  }

  outcome::result<void> RouterImpl::handle(
      const peer::ProtocolName &p, std::shared_ptr<connection::Stream> stream) {
    // firstly, try to find the longest prefix - even if it's not perfect match,
    // but a predicate one, it still will save the resources
    auto matched_proto = proto_handlers_.longest_prefix(p);
    if (matched_proto == proto_handlers_.end()) {
      return Error::NO_HANDLER_FOUND;
    }

    const auto &[predicate, cb] = matched_proto.value();
    auto matched = matched_proto.key() == p or (predicate and predicate(p));
    if (matched) {
      // perfect or predicate match
      cb(StreamAndProtocol{std::move(stream), matched_proto.key()});
      return outcome::success();
    }

    // fallback: find all matches for the first two letters of the given (the
    // first letter is a '/', so we need two) protocol and test them against the
    // predicate; the longest match is to be called
    auto matched_protos = proto_handlers_.equal_prefix_range_ks(p.data(), 2);

    auto longest_match{matched_protos.second};
    for (auto match = matched_protos.first; match != matched_protos.second;
         ++match) {
      if (match->predicate and match->predicate(p)
          and (longest_match == matched_protos.second
               or match.key().size() > longest_match.key().size())) {
        longest_match = match;
      }
    }

    if (longest_match == matched_protos.second) {
      return Error::NO_HANDLER_FOUND;
    }
    longest_match->handler(
        StreamAndProtocol{std::move(stream), longest_match.key()});
    return outcome::success();
  }

}  // namespace libp2p::network
