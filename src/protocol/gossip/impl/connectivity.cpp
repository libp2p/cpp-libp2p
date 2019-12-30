/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/connectivity.hpp>

#include <cassert>

#include <boost/range/algorithm/for_each.hpp>

#include <libp2p/protocol/gossip/impl/message_builder.hpp>
#include <libp2p/protocol/gossip/impl/message_receiver.hpp>

namespace libp2p::protocol::gossip {

  namespace {

    template <typename T>
    bool contains(const std::vector<T> &container, const T &element) {
      return (container.empty())
          ? false
          : (std::find(container.begin(), container.end(), element)
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
        log_("gossip", "Connectivity", this) {
    // clang-format off
    on_reader_event_ =
        [this, self_wptr=weak_from_this()]
        (const PeerContextPtr &from, outcome::result<Success> event) {
          if (self_wptr.expired()) return;
          onReaderEvent(from, event);
        };

    on_writer_event_ =
        [this, self_wptr=weak_from_this()]
        (const PeerContextPtr &from, outcome::result<Success> event) {
          if (self_wptr.expired()) return;
          onWriterEvent(from, event);
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

    log_.info("started");
  }

  Connectivity::~Connectivity() {
    stop();
  }

  void Connectivity::stop() {
    stopped_ = true;
    all_peers_.selectAll([](const PeerContextPtr &ctx) {
      if (ctx->writer) {
        ctx->writer->close();
      }
      if (ctx->reader) {
        ctx->reader->close();
      }
    });
    readers_.clear();
    connected_peers_.clear();
  }

  void Connectivity::addBootstrapPeer(
      peer::PeerId id, boost::optional<multi::Multiaddress> address) {
    auto ctx_found = all_peers_.find(id);

    PeerContextPtr ctx;
    if (ctx_found) {
      // peer is known, just update address
      ctx = ctx_found.value();
    } else {
      ctx = std::make_shared<PeerContext>(std::move(id));
      connectable_peers_.insert(ctx);
    }

    ctx->dial_to = std::move(address);
  }

  void Connectivity::flush(const PeerContextPtr &ctx) {
    assert(ctx);
    assert(ctx->message_to_send);

    if (stopped_) {
      return;
    }

    if (!ctx->writer) {
      // not yet connected, will be flushed next time
      return;
    }

    if (ctx->message_to_send->empty()) {
      // nothing to flush, it's ok
      return;
    }

    // N.B. errors, if any, will be passed later in async manner
    ctx->writer->write(ctx->message_to_send->serialize());
  }

  peer::Protocol Connectivity::getProtocolId() const {
    return config_.protocol_version;
  }

  void Connectivity::handle(StreamResult rstream) {
    if (stopped_) {
      return;
    }

    if (!rstream) {
      log_.info("incoming connection failed due to '{}'",
                rstream.error().message());
      return;
    }
    auto &stream = rstream.value();

    // no remote peer id means dead stream

    auto peer_res = stream->remotePeerId();
    if (!peer_res) {
      log_.info(" connection from '{}' failed: {}",
                stream->remoteMultiaddr().value().getStringAddress(),
                peer_res.error().message());
    } else {
      log_.debug(" connection from '{}', peer_id={}",
                 stream->remoteMultiaddr().value().getStringAddress(),
                 peer_res.value().toBase58());
    }
    auto &peer_id = peer_res.value();

    PeerContextPtr ctx;

    auto ctx_found = all_peers_.find(peer_id);
    if (!ctx_found) {
      if (readers_.size() >= config_.max_connections_num) {
        log_.debug("too many connections, refusing");
        return;
      }
      ctx = std::make_shared<PeerContext>(peer_id);
      // core may append messages before outbound stream establishes
      ctx->message_to_send = std::make_shared<MessageBuilder>();

      all_peers_.insert(ctx);

      // make outbound stream over existing connection
      dial(ctx, true);
    } else {
      ctx = std::move(ctx_found.value());
      if (!ctx->writer) {
        // not connected or connecting
        dial(ctx, true);
      }
    }

    // currently we prefer newer streams, but avoid duplicate ones,
    // because this is pub-sub and broadcast
    if (ctx->reader) {
      ctx->reader->close();
    } else {
      readers_.insert(ctx);
    }

    ctx->reader =
        std::make_shared<StreamReader>(config_, *scheduler_, on_reader_event_,
                                       *msg_receiver_, std::move(stream), ctx);
    ctx->reader->read();
  }

  void Connectivity::dial(const PeerContextPtr &ctx,
                          bool connection_must_exist) {
    using C = network::ConnectionManager::Connectedness;

    assert(!ctx->writer);
    assert(!connecting_peers_.contains(ctx->peer_id));

    if (ctx->banned_until != 0 && connection_must_exist) {
      // unban outbound connection only if inbound one exists
      unban(ctx);
    } else {
      connectable_peers_.erase(ctx->peer_id);
    }

    peer::PeerInfo pi = host_->getPeerRepository().getPeerInfo(ctx->peer_id);
    if (ctx->dial_to) {
      if (!contains(pi.addresses, ctx->dial_to.value())) {
        pi.addresses.push_back(ctx->dial_to.value());
      }
    }

    auto can_connect =
        host_->getNetwork().getConnectionManager().connectedness(pi);

    if (can_connect != C::CONNECTED && can_connect != C::CAN_CONNECT) {
      if (connection_must_exist) {
        log_.error("connection must exist but not found for {}",
                   ctx->peer_id.toBase58());
      } else {
        log_.debug("{} is not connectable at the moment",
                   ctx->peer_id.toBase58());
      }
      return;
    }

    // clang-format off
    host_->newStream(
        pi,
        config_.protocol_version,
        [wptr = weak_from_this(), this, p=ctx] (auto &&rstream) mutable {
            auto self = wptr.lock();
            if (self) {
                onConnected(
                    std::move(p), std::forward<decltype(rstream)>(rstream)
                );
            }
        }
    );
    // clang-format on

    connecting_peers_.insert(ctx);
  }

  void Connectivity::ban(PeerContextPtr ctx) {
    //  TODO(artem): lift this parameter up to some internal config
    constexpr Time kBanInterval = 60000;

    assert(ctx);

    auto ts = scheduler_->now() + kBanInterval;
    ctx->banned_until = ts;
    ctx->message_to_send->clear();
    ctx->writer.reset();
    banned_peers_expiration_.insert({ts, std::move(ctx)});
  }

  void Connectivity::unban(const PeerContextPtr &ctx) {
    auto ts = ctx->banned_until;

    assert(ts > 0);

    banned_peers_expiration_.erase({ts, ctx});
    ctx->banned_until = 0;
  }

  void Connectivity::onConnected(PeerContextPtr ctx, StreamResult rstream) {
    if (stopped_) {
      return;
    }

    auto ctx_found = connecting_peers_.erase(ctx->peer_id);
    if (!ctx_found) {
      log_.error("cannot find connecting peer {}", ctx->peer_id.toBase58());
      return;
    }

    if (!rstream) {
      log_.info("cannot connect, peer={}, error={}", ctx->peer_id.toBase58(),
                rstream.error().message());
      ban(std::move(ctx));
      return;
    }

    ctx->writer =
        std::make_shared<StreamWriter>(config_, *scheduler_, on_writer_event_,
                                       std::move(rstream.value()), ctx);

    connected_cb_(true, ctx);
  }

  void Connectivity::onReaderEvent(const PeerContextPtr &from,
                                   outcome::result<Success> event) {
    if (stopped_) {
      return;
    }

    if (event) {
      // do nothing at the moment, keep it connected
      return;
    }
    log_.info("inbound stream error='{}', peer={}", event.error().message(),
              from->peer_id.toBase58());

    // TODO(artem): ban incoming peers for protocol violations etc.

    from->reader->close();
    from->reader.reset();

    // let them connect once more if they want
    readers_.erase(from->peer_id);
  }

  void Connectivity::onWriterEvent(const PeerContextPtr &from,
                                   outcome::result<Success> event) {
    if (stopped_) {
      return;
    }

    if (event) {
      // do nothing at the moment, keep it connected
      return;
    }
    log_.info("outbound stream error='{}', peer={}", event.error().message(),
              from->peer_id.toBase58());

    if (!connected_peers_.erase(from->peer_id)) {
      log_.debug("peer not found for {}", from->peer_id.toBase58());
      return;
    }

    // TODO(artem): different ban intervals depending on error
    ban(from);

    connected_cb_(false, from);
  }

  void Connectivity::peerIsWritable(const PeerContextPtr &ctx,
                                    bool low_latency) {
    assert (!ctx->message_to_send->empty());

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
    if (stopped_) {
      return;
    }

    // unban connect candidates
    auto ts = scheduler_->now();
    while (!banned_peers_expiration_.empty()) {
      auto it = banned_peers_expiration_.begin();
      if (it->first > ts) {
        break;
      }
      unban(it->second);
      connectable_peers_.insert(it->second);
    }

    // connect if needed
    auto sz = connected_peers_.size();
    if (sz < config_.ideal_connections_num) {
      auto peers = connectable_peers_.selectRandomPeers(
          config_.ideal_connections_num - sz);
      for (auto &p : peers) {
        dial(p, false);
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
                ctx->message_to_send->addSubscription(p.first, p.second);
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

}  // namespace libp2p::protocol::gossip
