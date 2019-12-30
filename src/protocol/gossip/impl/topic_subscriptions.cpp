/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/topic_subscriptions.hpp>

#include <algorithm>

#include <libp2p/protocol/gossip/impl/connectivity.hpp>
#include <libp2p/protocol/gossip/impl/message_builder.hpp>

namespace libp2p::protocol::gossip {

  TopicSubscriptions::TopicSubscriptions(TopicId topic, const Config &config,
                                         Connectivity &connectivity)
      : topic_(std::move(topic)),
        config_(config),
        connectivity_(connectivity),
        self_subscribed_(false),
        fanout_period_ends_(0) {}

  bool TopicSubscriptions::empty() const {
    return (not self_subscribed_) && (fanout_period_ends_ == 0)
        && subscribed_peers_.empty() && mesh_peers_.empty();
  }

  void TopicSubscriptions::onNewMessage(const TopicMessage::Ptr &msg,
                                        const MessageId &msg_id,
                                        bool is_published_locally, Time now) {
    if (is_published_locally) {
      fanout_period_ends_ = now + config_.seen_cache_lifetime_msec;
    }

    mesh_peers_.selectAll([this, &msg, &msg_id](const PeerContextPtr &ctx) {
      assert(ctx->message_to_send);

      ctx->message_to_send->addMessage(*msg, msg_id);

      // forward immediately to those in mesh
      connectivity_.peerIsWritable(ctx, true);
    });

    subscribed_peers_.selectAll(
        [this, &msg_id, is_published_locally](const PeerContextPtr &ctx) {
          assert(ctx->message_to_send);

          ctx->message_to_send->addIHave(topic_, msg_id);

          // local messages announce themselves immediately
          connectivity_.peerIsWritable(ctx, is_published_locally);
        });

    seen_cache_.emplace_back(now, msg_id);
  }

  void TopicSubscriptions::onHeartbeat(Time now) {
    if (self_subscribed_ && !subscribed_peers_.empty()) {
      // add/remove mesh members according to desired network density D
      size_t sz = mesh_peers_.size();

      if (sz < config_.D) {
        auto peers = subscribed_peers_.selectRandomPeers(config_.D - sz);
        for (auto &p : peers) {
          addToMesh(p);
          subscribed_peers_.erase(p->peer_id);
        }
      } else if (sz > config_.D) {
        auto peers = mesh_peers_.selectRandomPeers(sz - config_.D);
        for (auto &p : peers) {
          removeFromMesh(p);
          mesh_peers_.erase(p->peer_id);
        }
      }
    }

    // fanout ends some time after this host ends publishing to the topic,
    // to save space and traffic
    if (fanout_period_ends_ < now) {
      fanout_period_ends_ = 0;
    }

    // shift msg ids cache
    if (!seen_cache_.empty()) {
      auto it = std::find_if(seen_cache_.begin(), seen_cache_.end(),
                             [now](const auto &p) { return p.first >= now; });
      seen_cache_.erase(seen_cache_.begin(), it);
    }
  }

  void TopicSubscriptions::onSelfSubscribed(bool self_subscribed) {
    self_subscribed_ = self_subscribed;
    if (!self_subscribed_) {
      // remove the mesh
      mesh_peers_.selectAll(
          [this](const PeerContextPtr &p) { removeFromMesh(p); });
      mesh_peers_.clear();
    }
  }

  void TopicSubscriptions::onPeerSubscribed(const PeerContextPtr &p) {
    if (p->subscribed_to.count(topic_) != 0) {
      // ignore double subscription
      return;
    }

    p->subscribed_to.insert(topic_);
    subscribed_peers_.insert(p);

    // announce the peer about messages available for the topic
    for (const auto &[_, msg_id] : seen_cache_) {
      p->message_to_send->addIHave(topic_, msg_id);
    }
    // will be sent on next heartbeat
    connectivity_.peerIsWritable(p, false);
  }

  void TopicSubscriptions::onPeerUnsubscribed(const PeerContextPtr &p) {
    auto res = subscribed_peers_.erase(p->peer_id);
    if (!res) {
      res = mesh_peers_.erase(p->peer_id);
    }
    if (res) {
      res.value()->subscribed_to.erase(topic_);
    }
  }

  void TopicSubscriptions::onGraft(const PeerContextPtr &p) {
    auto res = mesh_peers_.find(p->peer_id);
    if (res) {
      // already there
      return;
    }
    res = subscribed_peers_.find(p->peer_id);
    if (!res) {
      res.value()->subscribed_to.insert(topic_);
    }
    if (res) {
      if (self_subscribed_) {
        mesh_peers_.insert(std::move(res.value()));
      } else {
        // we don't have mesh for the topic
        p->message_to_send->addPrune(topic_);
        connectivity_.peerIsWritable(p, true);
      }
    }
  }

  void TopicSubscriptions::onPrune(const PeerContextPtr &p) {
    mesh_peers_.erase(p->peer_id);
  }

  void TopicSubscriptions::addToMesh(const PeerContextPtr &p) {
    assert(p->message_to_send);

    p->message_to_send->addGraft(topic_);
    connectivity_.peerIsWritable(p, false);
    mesh_peers_.insert(p);
  }

  void TopicSubscriptions::removeFromMesh(const PeerContextPtr &p) {
    assert(p->message_to_send);

    p->message_to_send->addPrune(topic_);
    connectivity_.peerIsWritable(p, false);
    subscribed_peers_.insert(p);
  }

}  // namespace libp2p::protocol::gossip
