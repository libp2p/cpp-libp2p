/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/find_peer_executor.hpp>

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  std::atomic_size_t FindPeerExecutor::instance_number = 0;

  FindPeerExecutor::FindPeerExecutor(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<SessionHost> session_host,
      std::shared_ptr<PeerRouting> peer_routing,
      const std::shared_ptr<PeerRoutingTable> &peer_routing_table,
      PeerId sought_peer_id, FoundPeerInfoHandler handler)
      : config_(config),
        host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        session_host_(std::move(session_host)),
        peer_routing_(std::move(peer_routing)),
        sought_peer_id_(std::move(sought_peer_id)),
        target_(sought_peer_id_),
        handler_(std::move(handler)),
        log_("KademliaExecutor", "kademlia", "FindPeer", ++instance_number) {
    auto nearest_peer_ids = peer_routing_table->getNearestPeers(
        target_, config_.closerPeerCount * 2);

    nearest_peer_ids_.insert(std::move_iterator(nearest_peer_ids.begin()),
                             std::move_iterator(nearest_peer_ids.end()));

    std::for_each(nearest_peer_ids_.begin(), nearest_peer_ids_.end(),
                  [this](auto &peer_id) { queue_.emplace(peer_id, target_); });

    log_.debug("created");
  }

  FindPeerExecutor::~FindPeerExecutor() {
    log_.debug("destroyed");
  }

  outcome::result<void> FindPeerExecutor::start() {
    if (started_) {
      return Error::IN_PROGRESS;
    }
    if (done_) {
      return Error::FULFILLED;
    }
    started_ = true;

    serialized_request_ = std::make_shared<std::vector<uint8_t>>();

    boost::optional<peer::PeerInfo> self_announce;
    if (not config_.passiveMode) {
      self_announce = host_->getPeerInfo();
    }

    Message request =
        createFindNodeRequest(sought_peer_id_, std::move(self_announce));
    if (!request.serialize(*serialized_request_)) {
      done_ = true;
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    log_.debug("started");

    scheduler_->schedule(
        [wp = weak_from_this()] {
          if (auto self = wp.lock()) {
            self->done(Error::TIMEOUT);
          }
        },
        config_.randomWalk.timeout);

    spawn();

    return outcome::success();
  };

  void FindPeerExecutor::done(outcome::result<PeerInfo> result) {
    bool x = false;
    if (not done_.compare_exchange_strong(x, true)) {
      return;
    }
    if (result.has_value()) {
      log_.debug("done: peer is found");
    } else {
      log_.debug("done: {}", result.error().message());
    }
    handler_(result);
  }

  void FindPeerExecutor::spawn() {
    if (done_) {
      return;
    }

    auto self_peer_id = host_->getId();

    while (started_ and not done_ and not queue_.empty()
           and requests_in_progress_ < config_.requestConcurency) {
      auto peer_id = *queue_.top();
      queue_.pop();

      // Exclude yoursef, because not found locally anyway
      if (peer_id == self_peer_id) {
        continue;
      }

      // Get peer info
      auto peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
      if (peer_info.addresses.empty()) {
        continue;
      }

      // Check if connectable
      auto connectedness = host_->connectedness(peer_info);
      if (connectedness == Message::Connectedness::CAN_NOT_CONNECT) {
        continue;
      }

      ++requests_in_progress_;

      log_.debug("connecting to {}; active {}, in queue {}", peer_id.toBase58(),
                 requests_in_progress_, queue_.size());

      auto holder =
          std::make_shared<std::pair<std::shared_ptr<FindPeerExecutor>,
                                     basic::Scheduler::Handle>>();

      holder->first = shared_from_this();
      holder->second = scheduler_->scheduleWithHandle(
          [holder] {
            if (holder->first) {
              holder->second.cancel();
              holder->first->onConnected(Error::TIMEOUT);
              holder->first.reset();
            }
          },
          config_.connectionTimeout);

      host_->newStream(
          peer_info, {config_.protocolId},
          [holder](auto &&stream_res) {
            if (holder->first) {
              holder->second.cancel();
              holder->first->onConnected(stream_res);
              holder->first.reset();
            }
          },
          config_.connectionTimeout);
    }

    if (requests_in_progress_ == 0) {
      done(Error::VALUE_NOT_FOUND);
    }
  }

  void FindPeerExecutor::onConnected(StreamAndProtocolOrError stream_res) {
    if (not stream_res) {
      --requests_in_progress_;

      log_.debug("cannot connect to peer: {}; active {}, in queue {}",
                 stream_res.error().message(), requests_in_progress_,
                 queue_.size());

      spawn();
      return;
    }

    auto &stream = stream_res.value().stream;
    assert(stream->remoteMultiaddr().has_value());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());

    log_.debug("connected to {}; active {}, in queue {}", addr,
               requests_in_progress_, queue_.size());

    log_.debug("outgoing stream with {}",
               stream->remotePeerId().value().toBase58());

    auto session = session_host_->openSession(stream);
    if (!session->write(serialized_request_, shared_from_this())) {
      --requests_in_progress_;

      log_.debug("write to {} failed; active {}, in queue {}", addr,
                 requests_in_progress_, queue_.size());

      spawn();
      return;
    }
  }

  Time FindPeerExecutor::responseTimeout() const {
    return config_.responseTimeout;
  }

  bool FindPeerExecutor::match(const Message &msg) const {
    return
        // Check if message type is appropriate
        msg.type == Message::Type::kFindNode;
    // Check if response is accorded to request
    // && msg.key == sought_peer_id_.toVector()
  }

  void FindPeerExecutor::onResult(const std::shared_ptr<Session> &session,
                                  outcome::result<Message> msg_res) {
    gsl::final_action respawn([this] {
      --requests_in_progress_;
      spawn();
    });

    // Check if gotten some message
    if (not msg_res) {
      log_.warn("Result from {} is failed: {}; active {}, in queue {}",
                session->stream()->remotePeerId().value().toBase58(),
                msg_res.error().message(), requests_in_progress_,
                queue_.size());
      return;
    }
    auto &msg = msg_res.value();

    // Skip inappropriate messages
    if (not match(msg)) {
      BOOST_UNREACHABLE_RETURN();
    }

    auto remote_peer_id_res = session->stream()->remotePeerId();
    BOOST_ASSERT(remote_peer_id_res.has_value());
    auto &remote_peer_id = remote_peer_id_res.value();

    auto self_peer_id = host_->getId();

    log_.debug("Result from {} is gotten; active {}, in queue {}",
               remote_peer_id.toBase58(), requests_in_progress_, queue_.size());

    // Append gotten peer to queue
    if (msg.closer_peers) {
      for (auto &peer : msg.closer_peers.value()) {
        // Skip non connectable peers
        if (peer.conn_status == Message::Connectedness::CAN_NOT_CONNECT) {
          continue;
        }

        // Add/Update peer info
        auto add_addr_res =
            host_->getPeerRepository().getAddressRepository().upsertAddresses(
                peer.info.id,
                gsl::span(peer.info.addresses.data(),
                          static_cast<
                              gsl::span<const multi::Multiaddress>::index_type>(
                              peer.info.addresses.size())),
                peer::ttl::kDay);
        if (not add_addr_res) {
          continue;
        }

        // It is done, just add peer and that's all
        if (done_) {
          continue;
        }

        // Found
        if (peer.info.id == sought_peer_id_) {
          done(peer.info);
        }

        // Skip himself
        if (peer.info.id == self_peer_id) {
          continue;
        }

        // Skip remote
        if (peer.info.id == remote_peer_id) {
          continue;
        }

        // New peer add to queue
        if (auto [it, ok] = nearest_peer_ids_.emplace(peer.info.id); ok) {
          queue_.emplace(*it, target_);
        }
      }
    }
  }

}  // namespace libp2p::protocol::kademlia
