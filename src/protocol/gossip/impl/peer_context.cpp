/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/peer_context.hpp>

namespace libp2p::protocol::gossip {

  namespace  {

    std::string makeStringRepr(const peer::PeerId& id) {
      return id.toBase58().substr(46);
    }

  } //namespace

  PeerContext::PeerContext(peer::PeerId id)
      : peer_id(std::move(id)), str(makeStringRepr(peer_id)) {}

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
