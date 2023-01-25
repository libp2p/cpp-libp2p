/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/get_value_executor.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/put_value_executor.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  std::atomic_size_t GetValueExecutor::instance_number = 0;

  GetValueExecutor::GetValueExecutor(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<SessionHost> session_host,
      std::shared_ptr<PeerRouting> peer_routing,
      std::shared_ptr<ContentRoutingTable> content_routing_table,
      const std::shared_ptr<PeerRoutingTable> &peer_routing_table,
      std::shared_ptr<ExecutorsFactory> executor_factory,
      std::shared_ptr<Validator> validator, ContentId key,
      FoundValueHandler handler)
      : config_(config),
        host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        session_host_(std::move(session_host)),
        peer_routing_(std::move(peer_routing)),
        content_routing_table_(std::move(content_routing_table)),
        executor_factory_(std::move(executor_factory)),
        validator_(std::move(validator)),
        key_(std::move(key)),
        handler_(std::move(handler)),
        target_(key_),
        log_("KademliaExecutor", "kademlia", "GetValue", ++instance_number) {
    BOOST_ASSERT(host_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(session_host_ != nullptr);
    BOOST_ASSERT(peer_routing_ != nullptr);
    BOOST_ASSERT(content_routing_table_ != nullptr);
    BOOST_ASSERT(executor_factory_ != nullptr);
    BOOST_ASSERT(validator_ != nullptr);

    auto nearest_peer_ids = peer_routing_table->getNearestPeers(
        target_, config_.closerPeerCount * 2);

    nearest_peer_ids_.insert(std::move_iterator(nearest_peer_ids.begin()),
                             std::move_iterator(nearest_peer_ids.end()));

    std::for_each(nearest_peer_ids_.begin(), nearest_peer_ids_.end(),
                  [this](auto &peer_id) { queue_.emplace(peer_id, target_); });

    received_records_ = std::make_unique<Table>();
    log_.debug("created");
  }

  GetValueExecutor::~GetValueExecutor() {
    log_.debug("destroyed");
  }

  outcome::result<void> GetValueExecutor::start() {
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

    Message request = createGetValueRequest(key_, std::move(self_announce));
    if (!request.serialize(*serialized_request_)) {
      done_ = true;
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    log_.debug("started");

    spawn();
    return outcome::success();
  };

  void GetValueExecutor::spawn() {
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

      log_.debug("connecting to {}; active {}, in queue {}",
                 peer_info.id.toBase58(), requests_in_progress_, queue_.size());

      auto holder =
          std::make_shared<std::pair<std::shared_ptr<GetValueExecutor>,
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

    if (done_) {
      return;
    }

    if (requests_in_progress_ == 0) {
      done_ = true;
      log_.debug("done");
      handler_(Error::VALUE_NOT_FOUND);
    }
  }

  void GetValueExecutor::onConnected(StreamAndProtocolOrError stream_res) {
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

  Time GetValueExecutor::responseTimeout() const {
    return config_.responseTimeout;
  }

  bool GetValueExecutor::match(const Message &msg) const {
    return
        // Check if message type is appropriate
        msg.type == Message::Type::kGetValue;
  }

  void GetValueExecutor::onResult(const std::shared_ptr<Session> &session,
                                  outcome::result<Message> msg_res) {
    if (done_) {
      return;
    }

    gsl::final_action respawn([this] {
      --requests_in_progress_;
      spawn();
    });

    // Check if gotten some message
    if (not msg_res) {
      log_.warn("Result from {} failed: {}; active {}, in queue {}",
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

    if (msg.record) {
      auto &value = msg.record.value().value;

      auto validation_res = validator_->validate(key_, value);
      if (not validation_res.has_value()) {
        log_.debug("Result from {} is invalid", remote_peer_id.toBase58());
        return;
      }

      received_records_->insert({remote_peer_id, value});

      if (received_records_->size() >= config_.valueLookupsQuorum) {
        std::vector<Value> values;
        std::transform(received_records_->begin(), received_records_->end(),
                       std::back_inserter(values),
                       [](auto &record) { return record.value; });

        auto index_res = validator_->select(key_, values);
        if (not index_res.has_value()) {
          log_.debug("Can't select best value of {} provided", values.size());
          return;
        }
        auto &best = values[index_res.value()];

        // Return result to upstear
        done_ = true;
        log_.debug("done");
        handler_(best);

        // Inform peer of new value
        std::vector<PeerId> addressees;
        auto &idx_by_value = received_records_->get<ByValue>();
        for (auto &[peer, value] : idx_by_value) {
          if (value != best) {
            addressees.emplace_back(peer);
          } else {
            content_routing_table_->addProvider(key_, peer);
          }
        }

        if (not addressees.empty()) {
          auto put_value_executor = executor_factory_->createPutValueExecutor(
              key_, std::move(value), std::move(addressees));
          [[maybe_unused]] auto res = put_value_executor->start();
        }
      }
    }
  }
}  // namespace libp2p::protocol::kademlia
