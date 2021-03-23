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
                             std::shared_ptr<Scheduler> scheduler,
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
            (const PeerContextPtr &from, outcome::result<Success> event) {
          if (self_wptr.expired()) return;
          onStreamEvent(from, event);
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
      peer::PeerId id, boost::optional<multi::Multiaddress> address) {
    if (id == host_->getId()) {
      return;
    }

    auto ctx_found = all_peers_.find(id);

    PeerContextPtr ctx;
    if (ctx_found) {
      // peer is known, just update address
      ctx = ctx_found.value();
    } else {
      ctx = std::make_shared<PeerContext>(std::move(id));
      all_peers_.insert(ctx);
      connectable_peers_.insert(ctx);
    }

    ctx->dial_to = std::move(address);
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
    if (!started_) {
      return;
    }

    if (!rstream) {
      log_.info("incoming connection failed, error={}",
                rstream.error().message());
      return;
    }

    onNewStream(std::move(rstream.value()), false);
  }

  void Connectivity::onNewStream(std::shared_ptr<connection::Stream> stream,
                                 bool is_outbound) {
    // no remote peer id means dead stream
    auto peer_res = stream->remotePeerId();
    if (!peer_res) {
      log_.info("ignoring dead stream: {}", peer_res.error().message());
      return;
    }

    auto &peer_id = peer_res.value();

    log_.debug("new {}bound stream, address={}, peer_id={}",
                 is_outbound ? "out" : "in",
                 stream->remoteMultiaddr().value().getStringAddress(),
                 peer_id.toBase58());

    PeerContextPtr ctx;

    auto ctx_found = all_peers_.find(peer_id);
    if (!ctx_found) {
      if (all_peers_.size() >= config_.max_connections_num) {
        log_.warn("too many connections, refusing new stream");
        stream->close([](outcome::result<void>) {});
        return;
      }

      ctx = std::make_shared<PeerContext>(peer_id);
      all_peers_.insert(ctx);
    } else {
      ctx = std::move(ctx_found.value());
      if (ctx->banned_until != 0) {
        // unban outbound connection only if inbound one exists
        unban(ctx);
      }
    }

    size_t stream_id = 0;
    bool is_new_connection = false;

    if (is_outbound) {
      assert(!ctx->outbound_stream);
      is_new_connection = ctx->inbound_streams.empty();
    } else {
      stream_id = ctx->inbound_streams.size() + 1;
      is_new_connection = (stream_id == 1 && !ctx->outbound_stream);
    }

    auto gossip_stream = std::make_shared<Stream>(
        stream_id, config_, *scheduler_, on_stream_event_, *msg_receiver_,
        std::move(stream), ctx);

    gossip_stream->read();

    if (is_outbound) {
      ctx->outbound_stream = std::move(gossip_stream);
    } else {
      ctx->inbound_streams.push_back(std::move(gossip_stream));
    }

    if (is_new_connection) {
      connected_peers_.insert(ctx);
      connected_cb_(true, ctx);
    }

    if (!ctx->outbound_stream) {
      // make stream for writing
      dial(ctx, true);
    } else {
      flush(ctx);
    }
  }

  void Connectivity::dial(const PeerContextPtr &ctx,
                          bool connection_must_exist) {
    using C = network::ConnectionManager::Connectedness;

    if (ctx->is_connecting || ctx->outbound_stream) {
      return;
    }

    if (ctx->banned_until != 0 && connection_must_exist) {
      // unban outbound connection only if inbound one exists
      unban(ctx);
    } else {
      connectable_peers_.erase(ctx->peer_id);
    }

    peer::PeerInfo pi = host_->getPeerRepository().getPeerInfo(ctx->peer_id);
    if (ctx->dial_to) {
      pi.addresses = {ctx->dial_to.value()};
    }

    auto can_connect =
        host_->getNetwork().getConnectionManager().connectedness(pi);

    if (can_connect != C::CONNECTED && can_connect != C::CAN_CONNECT) {
      if (connection_must_exist) {
        log_.error("connection must exist but not found for {}", ctx->str);
        return;
      }
      if (pi.addresses.empty()) {
        log_.debug("{} is not connectable at the moment", ctx->str);
        return;
      }
    }

    ctx->is_connecting = true;

    // clang-format off
    host_->newStream(
        pi,
        config_.protocol_version,
        [wptr = weak_from_this(), this, ctx=ctx] (auto &&rstream) mutable {
            auto self = wptr.lock();
            if (self) {
              ctx->is_connecting = false;
              if (!rstream) {
                log_.info("outbound connection failed, error={}",
                          rstream.error().message());
                ban(ctx);
                return;
              }
              onNewStream(std::move(rstream.value()), true);
            }
        }
    );
    // clang-format on
  }

  void Connectivity::ban(const PeerContextPtr &ctx) {
    //  TODO(artem): lift this parameter up to some internal config
    constexpr Time kBanInterval = 60000;

    assert(ctx);
    if (ctx->banned_until != 0) {
      return;
    }

    log_.info("banning peer {}, subscribed to {}", ctx->str,
              fmt::join(ctx->subscribed_to, ", "));

    auto ts = scheduler_->now() + kBanInterval;
    ctx->banned_until = ts;
    ctx->message_builder->clear();
    for (auto &s : ctx->inbound_streams) {
      s->close();
    }
    ctx->inbound_streams.clear();
    if (ctx->outbound_stream) {
      ctx->outbound_stream->close();
      ctx->outbound_stream.reset();
    }
    banned_peers_expiration_.insert({ts, ctx});
    connected_peers_.erase(ctx->peer_id);
    connectable_peers_.erase(ctx->peer_id);
    connected_cb_(false, ctx);
  }

  void Connectivity::unban(const PeerContextPtr &ctx) {
    auto ts = ctx->banned_until;

    assert(ts > 0);

    auto it = banned_peers_expiration_.find({ts, ctx});
    if (it == banned_peers_expiration_.end()) {
      log_.warn("cannot find banned peer {}", ctx->str);
      return;
    }

    unban(it);
  }

  void Connectivity::unban(BannedPeers::iterator it) {
    const auto& ctx = it->second;
    ctx->banned_until = 0;
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

    ban(from);
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
      connectable_peers_.insert(it->second);
      unban(it);
    }

    // connect if needed
    auto sz = connected_peers_.size();
    if (sz < config_.ideal_connections_num) {
      auto peers = connectable_peers_.selectRandomPeers(
          config_.ideal_connections_num - sz);
      for (auto &p : peers) {
        if (!p->outbound_stream) {
          log_.debug("dialing {}", p->str);
          dial(p, false);
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

      // clang-format off
      connected_peers_.selectAll(
          [&flat_changes, this] (const PeerContextPtr& ctx) {
            boost::for_each(flat_changes, [&ctx] (auto&& p) {
                ctx->message_builder->addSubscription(p.first, p.second);
            });
            flush(ctx);
          }
       );

    } else {
      flush();
      writable_peers_on_heartbeat_.selectAll(
          [this](const PeerContextPtr& ctx) {
            flush(ctx);
          }
      );
    }
    // clang-format on

    writable_peers_low_latency_.clear();
    writable_peers_on_heartbeat_.clear();
  }

  const PeerSet &Connectivity::getConnectedPeers() const {
    return connected_peers_;
  }

}  // namespace libp2p::protocol::gossip
