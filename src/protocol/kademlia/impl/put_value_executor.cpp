/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/put_value_executor.hpp>

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  std::atomic_size_t PutValueExecutor::instance_number = 0;

  PutValueExecutor::PutValueExecutor(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<SessionHost> session_host, ContentId key,
      ContentValue value, std::vector<PeerId> addressees)
      : config_(config),
        host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        session_host_(std::move(session_host)),
        key_(std::move(key)),
        value_(std::move(value)),
        addressees_(std::move(addressees)),
        log_("KademliaExecutor", "kademlia", "PutValue", ++instance_number) {
    log_.debug("created");
  }

  PutValueExecutor::~PutValueExecutor() {
    log_.debug("destroyed");
  }

  outcome::result<void> PutValueExecutor::start() {
    if (started_) {
      return Error::IN_PROGRESS;
    }
    if (done_) {
      return Error::FULFILLED;
    }
    started_ = true;

    serialized_request_ = std::make_shared<std::vector<uint8_t>>();

    Message request = createPutValueRequest(key_, value_);
    if (!request.serialize(*serialized_request_)) {
      done_ = true;
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    log_.debug("started");

    spawn();
    return outcome::success();
  };

  void PutValueExecutor::spawn() {
    while (started_ and not done_ and addressees_idx_ < addressees_.size()
           and requests_in_progress_ < config_.requestConcurency) {
      auto &peer_id = addressees_[addressees_idx_++];
      auto peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
      auto connectedness = host_->connectedness(peer_info);
      if (connectedness == Message::Connectedness::CAN_NOT_CONNECT) {
        continue;
      }

      ++requests_in_progress_;

      log_.debug("connecting to {}; active {}, in queue {}",
                 peer_info.id.toBase58(), requests_in_progress_,
                 addressees_.size() - addressees_idx_);

      auto holder =
          std::make_shared<std::pair<std::shared_ptr<PutValueExecutor>,
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
      done_ = true;
      log_.debug("done");
    }
  }

  void PutValueExecutor::onConnected(StreamAndProtocolOrError stream_res) {
    if (not stream_res) {
      --requests_in_progress_;

      log_.debug("cannot connect to peer: {}; active {}, in queue {}",
                 stream_res.error().message(), requests_in_progress_,
                 addressees_.size() - addressees_idx_);

      spawn();
      return;
    }

    auto &stream = stream_res.value().stream;
    assert(stream->remoteMultiaddr().has_value());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    log_.debug("connected to {}; done{}, active {}, in queue {}", addr,
               requests_succeed_, requests_in_progress_,
               addressees_.size() - addressees_idx_);

    log_.debug("outgoing stream with {}",
               stream->remotePeerId().value().toBase58());

    auto session = session_host_->openSession(stream);

    if (!session->write(serialized_request_, shared_from_this())) {
      --requests_in_progress_;
      log_.debug("write to {} failed; done {}, active {}, in queue {}", addr,
                 requests_succeed_, requests_in_progress_,
                 addressees_.size() - addressees_idx_);
      spawn();
    }
  }

  Time PutValueExecutor::responseTimeout() const {
    return config_.responseTimeout;
  }

  bool PutValueExecutor::match(const Message &msg) const {
    return
        // Check if message type is appropriate
        msg.type == Message::Type::kPutValue
        // Check if response is accorded to request
        && msg.key == key_;
  }

  void PutValueExecutor::onResult(const std::shared_ptr<Session> &session,
                                  outcome::result<Message> msg_res) {
    --requests_in_progress_;

    if (msg_res.has_error()) {
      log_.debug(
          "write to {} failed; done {}, active {}, in queue {}; error: {}",
          session->stream()->remotePeerId().value().toBase58(),
          requests_succeed_, requests_in_progress_,
          addressees_.size() - addressees_idx_, msg_res.error().message());
    } else {
      ++requests_succeed_;
      log_.debug("write to {} successfuly; done {}, active {}, in queue {}",
                 session->stream()->remotePeerId().value().toBase58(),
                 requests_succeed_, requests_in_progress_,
                 addressees_.size() - addressees_idx_);
    }

    spawn();
  }
}  // namespace libp2p::protocol::kademlia
