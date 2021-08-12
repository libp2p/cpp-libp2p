/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "connectivity.hpp"

#include <cassert>

#include <boost/range/algorithm/for_each.hpp>

#include "message_builder.hpp"
#include "message_receiver.hpp"

namespace libp2p::protocol::gossip {

  namespace {

    template <typename T>
    bool contains(const std::vector<T> &container, const T &element) {
      return !(container.empty())
          && (std::find(container.begin(), container.end(), element)
              != container.end());
    }

  }  // namespace

  Connectivity::Connectivity(Config config,
                             std::shared_ptr<basic::Scheduler> scheduler,
                             std::shared_ptr<Host> host,
                             std::shared_ptr<MessageReceiver> msg_receiver,
                             ConnectionStatusFeedback on_connected)
      : config_(std::move(config)),
        scheduler_(std::move(scheduler)),
        host_(std::move(host)),
        msg_receiver_(std::move(msg_receiver)),
        connected_cb_(std::move(on_connected)),
        log_("gossip", "Connectivity",
             host_->getPeerInfo().id.toBase58().substr(46)) {}

  Connectivity::~Connectivity() {
    stop();
  }

  void Connectivity::start() {
    if (started_) {
      return;
    }

    // clang-format off
    on_stream_event_ =
        [this, self_wptr=weak_from_this()]
            (PeerContextPtr from, outcome::result<Success> event) {
          if (self_wptr.expired()) return;
          onStreamEvent(std::move(from), event);
        };

    host_->setProtocolHandler(
        config_.protocol_version,
        [self_wptr=weak_from_this()]
            (protocol::BaseProtocol::StreamResult rstream) {
          auto h = self_wptr.lock();
          if (h) {
            h->handle(std::move(rstream));
          }
        }
    );
    // clang-format on

    started_ = true;

    log_.info("started");
  }

  void Connectivity::stop() {
    started_ = false;
    all_peers_.selectAll([](const PeerContextPtr &ctx) {
      for (auto &stream : ctx->inbound_streams) {
        stream->close();
      }
      ctx->inbound_streams.clear();
      if (ctx->outbound_stream) {
        ctx->outbound_stream->close();
        ctx->outbound_stream.reset();
      }
    });
    connected_peers_.clear();
    banned_peers_expiration_.clear();
  }

  void Connectivity::addBootstrapPeer(
      const peer::PeerId &id,
      const boost::optional<multi::Multiaddress> &address) {
    if (id == host_->getId()) {
      return;
    }

    if (address.has_value()) {
      gsl::span<const multi::Multiaddress> span(&address.value(), 1);
      std::ignore =
          host_->getPeerRepository().getAddressRepository().upsertAddresses(
              id, span, config_.address_expiration_msec);
    }

    if (!all_peers_.contains(id)) {
      auto ctx = std::make_shared<PeerContext>(id);
      all_peers_.insert(ctx);
      connectable_peers_.insert(ctx);
    }
  }

  void Connectivity::flush(const PeerContextPtr &ctx) const {
    assert(ctx);
    assert(ctx->message_builder);

    if (!started_) {
      return;
    }

    if (ctx->message_builder->empty()) {
      // nothing to flush, it's ok
      return;
    }

    if (!ctx->outbound_stream) {
      // will be flushed after connecting
      return;
    }

    // N.B. errors, if any, will be passed later in async manner
    auto serialized = ctx->message_builder->serialize();
    ctx->outbound_stream->write(std::move(serialized));
  }

  peer::Protocol Connectivity::getProtocolId() const {
    return config_.protocol_version;
  }

  void Connectivity::handle(StreamResult rstream) {
    if (!rstream) {
      log_.info("incoming connection failed, error={}",
                rstream.error().message());
      return;
    }

    if (!started_) {
      rstream.value()->reset();
      return;
    }

    auto &stream = rstream.value();

    // no remote peer id means dead stream
    auto peer_res = stream->remotePeerId();
    if (!peer_res) {
      log_.info("ignoring dead stream: {}", peer_res.error().message());
      return;
    }

    auto &peer_id = peer_res.value();

    log_.debug("new inbound stream, address={}, peer_id={}",
               stream->remoteMultiaddr().value().getStringAddress(),
               peer_id.toBase58());

    PeerContextPtr ctx;

    auto ctx_found = all_peers_.find(peer_id);
    if (!ctx_found) {
      if (connected_peers_.size() >= config_.max_connections_num) {
        log_.warn("too many connections, refusing new stream");
        stream->close([](outcome::result<void>) {});
        return;
      }

      ctx = std::make_shared<PeerContext>(peer_id);
      all_peers_.insert(ctx);
    } else {
      ctx = std::move(ctx_found.value());
      if (ctx->banned_until != Time::zero()) {
        // unban outbound connection only if inbound one exists
        unban(ctx);
      }
    }

    size_t stream_id = 0;
    bool is_new_connection = false;

    stream_id = ctx->inbound_streams.size() + 1;
    is_new_connection = (stream_id == 1 && !ctx->outbound_stream);

    auto gossip_stream = std::make_shared<Stream>(
        stream_id, config_, *scheduler_, on_stream_event_, *msg_receiver_,
        std::move(stream), ctx);

    gossip_stream->read();

    ctx->inbound_streams.push_back(std::move(gossip_stream));

    if (is_new_connection) {
      connected_peers_.insert(ctx);
      connected_cb_(true, ctx);
    }

    if (!ctx->outbound_stream) {
      // make stream for writing
      dialOverExistingConnection(ctx);
    } else {
      flush(ctx);
    }
  }

  void Connectivity::dial(const PeerContextPtr &ctx) {
    if (ctx->is_connecting || ctx->outbound_stream) {
      return;
    }

    connectable_peers_.erase(ctx->peer_id);

    peer::PeerInfo pi = host_->getPeerRepository().getPeerInfo(ctx->peer_id);
    auto can_connect = host_->connectedness(pi);

    if (can_connect != Host::Connectedness::CONNECTED
        && can_connect != Host::Connectedness::CAN_CONNECT) {
      log_.debug("{} is not connectable at the moment", ctx->str);
      banOrForget(ctx);
      return;
    }

    ctx->is_connecting = true;

    // clang-format off
    host_->newStream(
        pi,
        config_.protocol_version,
        [wptr = weak_from_this(), this, ctx=ctx] (auto &&rstream) mutable {
            auto self = wptr.lock();
          if (self) {
            onNewStream(ctx, std::move(rstream));
          }
        },
        config_.rw_timeout_msec
    );
    // clang-format on
  }

  void Connectivity::dialOverExistingConnection(const PeerContextPtr &ctx) {
    if (ctx->is_connecting || ctx->outbound_stream) {
      return;
    }

    ctx->is_connecting = true;

    // clang-format off
    host_->newStream(
        ctx->peer_id,
        config_.protocol_version,
        [wptr = weak_from_this(), this, ctx=ctx] (auto &&rstream) mutable {
          auto self = wptr.lock();
          if (self) {
            onNewStream(ctx, std::move(rstream));
          }
        }
    );
    // clang-format on
  }

  void Connectivity::onNewStream(
      const PeerContextPtr &ctx,
      outcome::result<std::shared_ptr<connection::Stream>> rstream) {
    ctx->is_connecting = false;

    assert(!ctx->outbound_stream);

    if (!rstream) {
      log_.info("outbound connection failed, error={}",
                rstream.error().message());
      if (started_) {
        banOrForget(ctx);
      }
      return;
    }

    auto &stream = rstream.value();

    if (!started_) {
      rstream.value()->reset();
      return;
    }

    // no remote peer id means dead stream
    auto peer_res = stream->remotePeerId();
    if (!peer_res) {
      log_.info("ignoring dead stream: {}", peer_res.error().message());
      banOrForget(ctx);
      return;
    }

    auto &peer_id = peer_res.value();

    log_.debug("new outbound stream, address={}, peer_id={}",
               stream->remoteMultiaddr().value().getStringAddress(),
               peer_id.toBase58());

    size_t stream_id = 0;
    bool is_new_connection = ctx->inbound_streams.empty();

    auto gossip_stream = std::make_shared<Stream>(
        stream_id, config_, *scheduler_, on_stream_event_, *msg_receiver_,
        std::move(stream), ctx);

    gossip_stream->read();

    ctx->outbound_stream = std::move(gossip_stream);

    if (is_new_connection) {
      connected_peers_.insert(ctx);
      connected_cb_(true, ctx);
    }

    flush(ctx);
  }

  void Connectivity::banOrForget(const PeerContextPtr &ctx) {
    assert(ctx);

    if (ctx->banned_until != Time::zero()) {
      return;
    }

    ctx->message_builder->reset();
    for (auto &s : ctx->inbound_streams) {
      s->close();
    }
    ctx->inbound_streams.clear();
    if (ctx->outbound_stream) {
      ctx->outbound_stream->close();
      ctx->outbound_stream.reset();
    }
    connected_peers_.erase(ctx->peer_id);
    connectable_peers_.erase(ctx->peer_id);
    connected_cb_(false, ctx);

    if (++ctx->dial_attempts > config_.max_dial_attempts) {
      log_.info("removing peer {}", ctx->str);
      if (ctx.use_count() > 2) {
        log_.warn("{} extra links to peer still remain", ctx.use_count() - 1);
      }
      all_peers_.erase(ctx->peer_id);
    } else {
      log_.info("banning peer {}, dial_attempts={}", ctx->str,
                ctx->dial_attempts);
      auto ts = scheduler_->now() + config_.ban_interval_msec;
      ctx->banned_until = ts;
      banned_peers_expiration_.insert({ts, ctx});
    }
  }

  void Connectivity::unban(const PeerContextPtr &ctx) {
    auto ts = ctx->banned_until;

    assert(ts > Time::zero());

    auto it = banned_peers_expiration_.find({ts, ctx});
    if (it == banned_peers_expiration_.end()) {
      log_.warn("cannot find banned peer {}", ctx->str);
      return;
    }

    unban(it);
  }

  void Connectivity::unban(BannedPeers::iterator it) {
    const auto &ctx = it->second;
    ctx->banned_until = Time::zero();
    log_.info("unbanning peer {}", ctx->str);
    banned_peers_expiration_.erase(it);
  }

  void Connectivity::onStreamEvent(const PeerContextPtr &from,
                                   outcome::result<Success> event) {
    if (!started_) {
      return;
    }

    if (event) {
      // do nothing at the moment, keep it connected
      return;
    }
    log_.info("stream error='{}', peer={}", event.error().message(), from->str);

    // TODO(artem): ban incoming peers for protocol violations etc. - v.1.1

    banOrForget(from);
  }

  void Connectivity::peerIsWritable(const PeerContextPtr &ctx,
                                    bool low_latency) {
    if (ctx->message_builder->empty()) {
      return;
    }

    if (low_latency) {
      writable_peers_low_latency_.insert(ctx);
    } else {
      writable_peers_on_heartbeat_.insert(ctx);
    }
  }

  void Connectivity::flush() {
    writable_peers_low_latency_.selectAll(
        [this](const PeerContextPtr &ctx) { flush(ctx); });
    writable_peers_low_latency_.clear();
  }

  void Connectivity::onHeartbeat(const std::map<TopicId, bool> &local_changes) {
    if (!started_) {
      return;
    }

    // unban connect candidates
    auto ts = scheduler_->now();
    while (!banned_peers_expiration_.empty()) {
      auto it = banned_peers_expiration_.begin();
      if (it->first > ts) {
        break;
      }

      auto ctx = it->second;

      unban(it);

      log_.debug("dialing unbanned {}", ctx->str);
      dial(ctx);
    }

    // connect if needed
    auto sz = connected_peers_.size();
    if (sz < config_.ideal_connections_num) {
      auto peers = connectable_peers_.selectRandomPeers(
          config_.ideal_connections_num - sz);
      for (auto &p : peers) {
        if (!p->outbound_stream) {
          log_.debug("dialing {}", p->str);
          dial(p);
        }
      }
    }

    if (!local_changes.empty()) {
      // we have something to say to all connected peers

      std::vector<std::pair<bool, TopicId>> flat_changes;
      flat_changes.reserve(local_changes.size());
      boost::for_each(local_changes, [&flat_changes](auto &&p) {
        flat_changes.emplace_back(p.second, std::move(p.first));
      });

      connected_peers_.selectAll(
          [&flat_changes, this](const PeerContextPtr &ctx) {
            boost::for_each(flat_changes, [&ctx](auto &&p) {
              ctx->message_builder->addSubscription(p.first, p.second);
            });
            flush(ctx);
          });

    } else {
      flush();
      writable_peers_on_heartbeat_.selectAll(
          [this](const PeerContextPtr &ctx) { flush(ctx); });
    }

    writable_peers_low_latency_.clear();
    writable_peers_on_heartbeat_.clear();

    if (ts >= addresses_renewal_time_) {
      addresses_renewal_time_ =
          scheduler_->now() + config_.address_expiration_msec * 9 / 10;
      auto &addr_repo = host_->getPeerRepository().getAddressRepository();
      connected_peers_.selectAll([this, &addr_repo](const PeerContextPtr &ctx) {
        std::ignore = addr_repo.updateAddresses(
            ctx->peer_id, config_.address_expiration_msec);
      });
    }
  }

  const PeerSet &Connectivity::getConnectedPeers() const {
    return connected_peers_;
  }

}  // namespace libp2p::protocol::gossip
