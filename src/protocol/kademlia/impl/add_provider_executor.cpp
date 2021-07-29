/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/add_provider_executor.hpp>

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  std::atomic_size_t AddProviderExecutor::instance_number = 0;

  AddProviderExecutor::AddProviderExecutor(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<SessionHost> session_host,
      const std::shared_ptr<PeerRoutingTable> &peer_routing_table,
      ContentId key)
      : config_(config),
        host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        session_host_(std::move(session_host)),
        key_(std::move(key)),
        target_(key_),
        log_("KademliaExecutor", "kademlia", "AddProvider", ++instance_number) {
    auto nearest_peer_ids =
        peer_routing_table->getNearestPeers(target_, config_.maxBucketSize);

    nearest_peer_ids_.insert(std::move_iterator(nearest_peer_ids.begin()),
                             std::move_iterator(nearest_peer_ids.end()));

    std::for_each(nearest_peer_ids_.begin(), nearest_peer_ids_.end(),
                  [this](auto &peer_id) { queue_.emplace(peer_id, target_); });

    log_.debug("created");
  }

  AddProviderExecutor::~AddProviderExecutor() {
    log_.debug("destroyed");
  }

  outcome::result<void> AddProviderExecutor::start() {
    if (started_) {
      return Error::IN_PROGRESS;
    }
    if (done_) {
      return Error::FULFILLED;
    }
    started_ = true;

    serialized_request_ = std::make_shared<std::vector<uint8_t>>();

    auto self_peer_info = host_->getPeerInfo();

    Message request = createAddProviderRequest(std::move(self_peer_info), key_);
    if (!request.serialize(*serialized_request_)) {
      done_ = true;
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    log_.debug("started");

    scheduler_->schedule(
        [wp = weak_from_this()] {
          if (auto self = wp.lock()) {
            self->done();
          }
        },
        config_.randomWalk.timeout);

    spawn();
    return outcome::success();
  };

  void AddProviderExecutor::done() {
    bool x = false;
    if (not done_.compare_exchange_strong(x, true)) {
      return;
    }

    log_.debug("done: broabcast to {} peers", requests_succeed_);
  }

  void AddProviderExecutor::spawn() {
    if (done_) {
      return;
    }

    auto self_peer_id = host_->getId();

    while (started_ and not done_ and not queue_.empty()
           and requests_in_progress_ < config_.requestConcurency
           and requests_succeed_ < config_.closerPeerCount) {
      auto peer_id = *queue_.top();
      queue_.pop();

      // Exclude yourself, because already known
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

      log_.debug("connecting to {}; done {}, active {}, in queue {}",
                 peer_info.id.toBase58(), requests_succeed_,
                 requests_in_progress_, queue_.size());

      auto holder =
          std::make_shared<std::pair<std::shared_ptr<AddProviderExecutor>,
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
          peer_info, config_.protocolId,
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
      done();
    }
  }

  void AddProviderExecutor::onConnected(
      outcome::result<std::shared_ptr<connection::Stream>> stream_res) {
    if (not stream_res) {
      --requests_in_progress_;

      log_.debug("cannot connect to peer: {}; done {}, active {}, in queue {}",
                 stream_res.error().message(), requests_succeed_,
                 requests_in_progress_, queue_.size());

      spawn();
      return;
    }

    auto &stream = stream_res.value();
    assert(stream->remoteMultiaddr().has_value());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    log_.debug("connected to {}; done {}, active {}, in queue {}", addr,
               requests_succeed_, requests_in_progress_, queue_.size());

    log_.debug("outgoing stream with {}",
               stream->remotePeerId().value().toBase58());

    auto session = session_host_->openSession(stream);

    --requests_in_progress_;

    if (session->write(serialized_request_, {})) {
      ++requests_succeed_;
      log_.debug("write to {} successfuly; done {}, active {}, in queue {}",
                 addr, requests_succeed_, requests_in_progress_, queue_.size());
    } else {
      log_.debug("write to {} failed; done {}, active {}, in queue {}", addr,
                 requests_succeed_, requests_in_progress_, queue_.size());
    }

    spawn();
  }
}  // namespace libp2p::protocol::kademlia
