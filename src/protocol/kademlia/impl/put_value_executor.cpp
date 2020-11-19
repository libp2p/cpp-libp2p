/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/put_value_executor.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  // NOLINTNEXTIME(cppcoreguidelines-avoid-non-const-global-variables)
  std::atomic_size_t PutValueExecutor::instance_number = 0;

  PutValueExecutor::PutValueExecutor(const Config &config,
                                     std::shared_ptr<Host> host,
                                     std::shared_ptr<SessionHost> session_host,
                                     ContentId key, ContentValue value,
                                     std::vector<PeerId> addressees)
      : config_(config),
        host_(std::move(host)),
        session_host_(std::move(session_host)),
        key_(std::move(key)),
        value_(std::move(value)),
        addressees_(std::move(addressees)),
        log_("kad", "PutValueExecutor", ++instance_number) {
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
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    started_ = true;

    log_.debug("started");

    spawn();
    return outcome::success();
  };

  void PutValueExecutor::spawn() {
    while (started_ and not done_ and addressees_idx_ < addressees_.size()
           and requests_in_progress_ < config_.requestConcurency) {
      auto &peer_id = addressees_[addressees_idx_++];
      auto peer_info =
          host_->getPeerRepository().getPeerInfo(std::move(peer_id));
      auto connectedness =
          host_->getNetwork().getConnectionManager().connectedness(peer_info);
      if (connectedness == Message::Connectedness::CAN_NOT_CONNECT) {
        continue;
      }

      ++requests_in_progress_;

      log_.debug("connecting to {}; active {} req.", peer_info.id.toBase58(),
                 requests_in_progress_);

      host_->newStream(
          peer_info, config_.protocolId,
          [self = shared_from_this()](auto &&stream_res) {
            self->onConnected(std::forward<decltype(stream_res)>(stream_res));
          });
    }

    if (requests_in_progress_ == 0) {
      done_ = true;
      log_.debug("done");
    }
  }

  void PutValueExecutor::onConnected(
      outcome::result<std::shared_ptr<connection::Stream>> stream_res) {
    if (not stream_res) {
      --requests_in_progress_;

      log_.debug("cannot connect to peer: {}; active {} req.",
                 stream_res.error().message(), requests_in_progress_);

      spawn();
      return;
    }

    auto &stream = stream_res.value();
    assert(stream->remoteMultiaddr().has_value());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    log_.debug("connected to {}; active {} req.", addr, requests_in_progress_);

    auto session = session_host_->openSession(stream);

    if (!session->write(serialized_request_, {})) {
      --requests_in_progress_;

      log_.warn("write to {} failed; active {} req.", addr,
                requests_in_progress_);
    }

    spawn();
  }
}  // namespace libp2p::protocol::kademlia
