/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "topic_subscriptions.hpp"

#include <algorithm>
#include <cassert>
#include <libp2p/protocol/gossip/explicit_peers.hpp>
#include <libp2p/protocol/gossip/score.hpp>

#include "choose_peers.hpp"
#include "connectivity.hpp"
#include "message_builder.hpp"

namespace libp2p::protocol::gossip {

  namespace {

    // dont forward message to peer it was received from as well as to its
    // original issuer
    bool needToForward(const PeerContextPtr &ctx,
                       const boost::optional<PeerContextPtr> &from,
                       const outcome::result<peer::PeerId> &origin) {
      if (from && ctx->peer_id == from.value()->peer_id) {
        return false;
      }
      return !(origin && ctx->peer_id == origin.value());
    }

  }  // namespace

  TopicSubscriptions::TopicSubscriptions(
      TopicId topic,
      const Config &config,
      Connectivity &connectivity,
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<ChoosePeers> choose_peers,
      std::shared_ptr<ExplicitPeers> explicit_peers,
      std::shared_ptr<Score> score,
      log::SubLogger &log)
      : topic_(std::move(topic)),
        config_(config),
        connectivity_(connectivity),
        scheduler_{std::move(scheduler)},
        choose_peers_{std::move(choose_peers)},
        explicit_peers_{std::move(explicit_peers)},
        score_{std::move(score)},
        self_subscribed_(false),
        log_(log) {
    connectivity_.getConnectedPeers().selectIf(
        [&](const PeerContextPtr &ctx) { subscribed_peers_.insert(ctx); },
        [&](const PeerContextPtr &ctx) {
          return ctx->subscribed_to.count(topic_) != 0;
        });
    if (config_.history_gossip == 0) {
      throw std::logic_error{"gossip config.history_gossip must not be zero"};
    }
    history_gossip_.resize(config_.history_gossip);
  }

  bool TopicSubscriptions::isUsed() const {
    return self_subscribed_ or fanout_.has_value();
  }

  bool TopicSubscriptions::isSubscribed() const {
    return self_subscribed_;
  }

  void TopicSubscriptions::onNewMessage(
      const boost::optional<PeerContextPtr> &from,
      const TopicMessage::Ptr &msg,
      const MessageId &msg_id,
      Time now) {
    bool is_published_locally = !from.has_value();

    if (not self_subscribed_ and not is_published_locally) {
      return;
    }

    auto origin = peerFrom(*msg);

    auto add_peer = [&](const PeerContextPtr &ctx) {
      if (needToForward(ctx, from, origin)
          and not ctx->idontwant.contains(msg_id)) {
        ctx->message_builder->addMessage(*msg, msg_id);
        connectivity_.peerIsWritable(ctx);
      }
    };

    if (config_.flood_publish and is_published_locally) {
      for (auto &ctx : subscribed_peers_) {
        if (explicit_peers_->contains(ctx->peer_id)
            or not score_->below(ctx->peer_id,
                                 config_.score.publish_threshold)) {
          add_peer(ctx);
        }
      }
    } else {
      for (auto &ctx : subscribed_peers_) {
        if (ctx->isFloodsub()
            and not score_->below(ctx->peer_id,
                                  config_.score.publish_threshold)) {
          add_peer(ctx);
        }
      }

      if (self_subscribed_) {
        for (auto &ctx : mesh_peers_) {
          add_peer(ctx);
        }
      } else {
        if (not fanout_.has_value()) {
          fanout_.emplace();
        }
        if (fanout_->peers.empty()) {
          fanout_->peers.extend(choose_peers_->choose(
              subscribed_peers_,
              [&](const PeerContextPtr &ctx) {
                return not explicit_peers_->contains(ctx->peer_id)
                   and not score_->below(ctx->peer_id,
                                         config_.score.publish_threshold);
              },
              config_.D));
        }
        fanout_->until = now + config_.fanout_ttl;
        for (auto &ctx : fanout_->peers) {
          add_peer(ctx);
        }
      }
    }

    history_gossip_.back().emplace_back(msg_id);

    if ((not is_published_locally or config_.idontwant_on_publish)
        and MessageBuilder::pbSize(*msg)
                > config_.idontwant_message_size_threshold) {
      for (auto &ctx : mesh_peers_) {
        if (ctx->isGossipsubv1_2()) {
          ctx->message_builder->addIDontWant(msg_id);
          connectivity_.peerIsWritable(ctx);
        }
      }
    }
  }

  void TopicSubscriptions::onHeartbeat(Time now) {
    auto slack = config_.backoff_slack * config_.heartbeat_interval_msec;
    for (auto it = dont_bother_until_.begin();
         it != dont_bother_until_.end();) {
      if (it->second + slack > now) {
        ++it;
      } else {
        it = dont_bother_until_.erase(it);
      }
    }

    if (self_subscribed_) {
      // add/remove mesh members according to desired network density D
      mesh_peers_.eraseIf([&](const PeerContextPtr &ctx) {
        if (score_->below(ctx->peer_id, config_.score.zero)) {
          sendPrune(ctx, false);
          return true;
        }
        return false;
      });
      if (mesh_peers_.size() < config_.D_min) {
        for (auto &ctx : choose_peers_->choose(
                 subscribed_peers_,
                 [&](const PeerContextPtr &ctx) {
                   return not mesh_peers_.contains(ctx)
                      and not explicit_peers_->contains(ctx->peer_id)
                      and not isBackoffWithSlack(ctx->peer_id)
                      and not score_->below(ctx->peer_id, config_.score.zero);
                 },
                 config_.D - mesh_peers_.size())) {
          addToMesh(ctx);
        }
      } else if (mesh_peers_.size() > config_.D_max) {
        auto peers =
            mesh_peers_.selectRandomPeers(mesh_peers_.size() - config_.D_max);
        for (auto &p : peers) {
          sendPrune(p, false);
          mesh_peers_.erase(p->peer_id);
        }
      }
    }

    // fanout ends some time after this host ends publishing to the topic,
    // to save space and traffic
    if (fanout_.has_value() and fanout_->until < now) {
      fanout_.reset();
      log_.debug("fanout period reset for {}", topic_);
    }
    if (fanout_.has_value()) {
      fanout_->peers.eraseIf([&](const PeerContextPtr &ctx) {
        return score_->below(ctx->peer_id, config_.score.publish_threshold);
      });
      if (fanout_->peers.size() < config_.D) {
        fanout_->peers.extend(choose_peers_->choose(
            subscribed_peers_,
            [&](const PeerContextPtr &ctx) {
              return not fanout_->peers.contains(ctx)
                 and not explicit_peers_->contains(ctx->peer_id)
                 and not score_->below(ctx->peer_id,
                                       config_.score.publish_threshold);
            },
            config_.D - fanout_->peers.size()));
      }
    }

    emitGossip();

    // shift msg ids cache
    history_gossip_.pop_front();
    history_gossip_.emplace_back();
  }

  void TopicSubscriptions::subscribe() {
    if (self_subscribed_) {
      return;
    }
    self_subscribed_ = true;
    if (fanout_.has_value()) {
      for (auto &ctx : fanout_->peers) {
        if (explicit_peers_->contains(ctx->peer_id)) {
          continue;
        }
        if (score_->below(ctx->peer_id, config_.score.publish_threshold)) {
          continue;
        }
        if (isBackoffWithSlack(ctx->peer_id)) {
          continue;
        }
        if (mesh_peers_.size() >= config_.D) {
          break;
        }
        addToMesh(ctx);
      }
      fanout_.reset();
    }
    if (mesh_peers_.size() < config_.D) {
      for (auto &ctx : choose_peers_->choose(
               subscribed_peers_,
               [&](const PeerContextPtr &ctx) {
                 return not mesh_peers_.contains(ctx)
                    and not explicit_peers_->contains(ctx->peer_id)
                    and not isBackoffWithSlack(ctx->peer_id)
                    and not score_->below(ctx->peer_id, config_.score.zero);
               },
               config_.D - mesh_peers_.size())) {
        addToMesh(ctx);
      }
    }
  }

  void TopicSubscriptions::unsubscribe() {
    if (not self_subscribed_) {
      return;
    }
    self_subscribed_ = false;
    for (auto &ctx : mesh_peers_) {
      sendPrune(ctx, true);
    }
    mesh_peers_.clear();
  }

  void TopicSubscriptions::onPeerSubscribed(const PeerContextPtr &ctx) {
    assert(ctx->subscribed_to.count(topic_) != 0);

    subscribed_peers_.insert(ctx);

    if (ctx->isGossipsub() and mesh_peers_.size() < config_.D_min
        and not mesh_peers_.contains(ctx)
        and not explicit_peers_->contains(ctx->peer_id)
        and not isBackoffWithSlack(ctx->peer_id)
        and not score_->below(ctx->peer_id, config_.score.zero)) {
      addToMesh(ctx);
    }
  }

  void TopicSubscriptions::onPeerUnsubscribed(const PeerContextPtr &ctx) {
    subscribed_peers_.erase(ctx->peer_id);
    if (fanout_) {
      fanout_->peers.erase(ctx->peer_id);
      if (fanout_->peers.empty()) {
        fanout_.reset();
      }
    }
    if (mesh_peers_.erase(ctx->peer_id)) {
      score_->prune(ctx->peer_id, topic_);
      updateBackoff(ctx->peer_id, config_.prune_backoff);
    }
  }

  void TopicSubscriptions::onGraft(const PeerContextPtr &ctx) {
    auto res = mesh_peers_.find(ctx->peer_id);
    if (res) {
      // already there
      return;
    }

    // implicit subscribe on graft
    subscribed_peers_.insert(ctx);

    bool mesh_is_full = (mesh_peers_.size() >= config_.D_max);

    bool prune = [&] {
      if (self_subscribed_) {
        return true;
      }
      if (explicit_peers_->contains(ctx->peer_id)) {
        return true;
      }
      if (isBackoff(ctx->peer_id, std::chrono::milliseconds{0})) {
        score_->addPenalty(ctx->peer_id, 1);
        if (isBackoff(ctx->peer_id,
                      config_.graft_flood_threshold - config_.prune_backoff)) {
          score_->addPenalty(ctx->peer_id, 1);
        }
      }
      if (score_->below(ctx->peer_id, config_.score.zero)) {
        return true;
      }
      if (mesh_is_full) {
        return true;
      }
      score_->graft(ctx->peer_id, topic_);
      mesh_peers_.insert(ctx);
      return false;
    }();
    if (prune) {
      sendPrune(ctx, false);
    }
  }

  void TopicSubscriptions::onPrune(
      const PeerContextPtr &ctx, std::optional<std::chrono::seconds> backoff) {
    if (mesh_peers_.erase(ctx->peer_id)) {
      score_->prune(ctx->peer_id, topic_);
    }
    updateBackoff(ctx->peer_id, backoff.value_or(config_.prune_backoff));
  }

  void TopicSubscriptions::addToMesh(const PeerContextPtr &ctx) {
    assert(ctx->message_builder);

    ctx->message_builder->addGraft(topic_);
    connectivity_.peerIsWritable(ctx);
    score_->graft(ctx->peer_id, topic_);
    mesh_peers_.insert(ctx);
    log_.debug("peer {} added to mesh (size={}) for topic {}",
               ctx->str,
               mesh_peers_.size(),
               topic_);
  }

  void TopicSubscriptions::sendPrune(const PeerContextPtr &ctx,
                                     bool unsubscribe) {
    assert(ctx->message_builder);

    auto backoff = unsubscribe ? config_.prune_backoff : config_.prune_backoff;
    updateBackoff(ctx->peer_id, backoff);
    ctx->message_builder->addPrune(
        topic_,
        ctx->isGossipsubv1_1() ? std::make_optional(backoff) : std::nullopt);
    score_->prune(ctx->peer_id, topic_);
    connectivity_.peerIsWritable(ctx);
    log_.debug("peer {} removed from mesh (size={}) for topic {}",
               ctx->str,
               mesh_peers_.size(),
               topic_);
  }

  bool TopicSubscriptions::isBackoff(const PeerId &peer_id,
                                     std::chrono::milliseconds slack) const {
    auto it = dont_bother_until_.find(peer_id);
    if (it == dont_bother_until_.end()) {
      return false;
    }
    return it->second + slack > scheduler_->now();
  }

  bool TopicSubscriptions::isBackoffWithSlack(const PeerId &peer_id) const {
    return isBackoff(peer_id,
                     config_.backoff_slack * config_.heartbeat_interval_msec);
  }

  void TopicSubscriptions::updateBackoff(const PeerId &peer_id,
                                         std::chrono::milliseconds duration) {
    auto until = scheduler_->now() + duration;
    auto [it, inserted] = dont_bother_until_.emplace(peer_id, until);
    if (not inserted and it->second < until) {
      it->second = until;
    }
  }

  void TopicSubscriptions::emitGossip() {
    auto &mesh = fanout_.has_value() ? fanout_->peers : mesh_peers_;
    auto peers = choose_peers_->choose(
        subscribed_peers_,
        [&](const PeerContextPtr &ctx) {
          return not mesh.contains(ctx)
             and not explicit_peers_->contains(ctx->peer_id)
             and not score_->below(ctx->peer_id,
                                   config_.score.gossip_threshold);
        },
        [&](size_t n) {
          return std::max<size_t>(config_.D_lazy, n * config_.gossip_factor);
        });
    size_t message_count = 0;
    for (auto &x : history_gossip_) {
      message_count += x.size();
    }
    if (message_count == 0) {
      return;
    }
    std::vector<MessageId> messages;
    messages.reserve(message_count);
    for (auto &x : history_gossip_) {
      messages.insert(messages.end(), x.begin(), x.end());
    }
    std::ranges::shuffle(messages, gossip_random_);
    for (auto &ctx : peers) {
      std::span messages_span{messages};
      if (messages.size() > config_.max_ihave_length) {
        std::ranges::shuffle(messages, gossip_random_);
        messages_span = messages_span.first(config_.max_ihave_length);
      }
      for (auto &message_id : messages_span) {
        ctx->message_builder->addIHave(topic_, message_id);
      }
      connectivity_.peerIsWritable(ctx);
    }
  }
}  // namespace libp2p::protocol::gossip
