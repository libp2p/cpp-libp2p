/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "peer_context.hpp"

#include "message_builder.hpp"

#include <libp2p/log/logger.hpp>

namespace libp2p::protocol::gossip {

  namespace {
    static auto &log() {
      static auto log{log::createLogger("gossip")};
      return log;
    }

    std::string makeStringRepr(const peer::PeerId &id) {
      auto str{id.toBase58()};
      constexpr size_t n{6};
      if (str.size() < n) {
        log()->warn("PeerContext PEER_ID_TOO_SHORT {}", str);
        return str + ":PEER_ID_TOO_SHORT";
      }
      return str.substr(str.size() - n);
    }

  }  // namespace

  PeerContext::~PeerContext() {
    log()->info("~PeerContext({})", str);
  }

  PeerContext::PeerContext(peer::PeerId id)
      : peer_id(std::move(id)),
        str(makeStringRepr(peer_id)),
        message_builder(std::make_shared<MessageBuilder>()) {}

  bool operator<(const PeerContextPtr &ctx, const peer::PeerId &peer) {
    if (!ctx)
      return false;
    return less(ctx->peer_id, peer);
  }

  bool operator<(const peer::PeerId &peer, const PeerContextPtr &ctx) {
    if (!ctx)
      return false;
    return less(peer, ctx->peer_id);
  }

  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b) {
    if (!a || !b)
      return false;
    return less(a->peer_id, b->peer_id);
  }

}  // namespace libp2p::protocol::gossip
