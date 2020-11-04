/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/find_providers_executor.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  std::atomic_size_t FindProvidersExecutor::instance_number = 0;

  FindProvidersExecutor::FindProvidersExecutor(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<KademliaImpl> kademlia,
      boost::optional<PeerInfo> self_peer_info, Key sought_key,
      std::unordered_set<PeerInfo> nearest_peer_infos,
      FoundProvidersHandler handler)
      : config_(config),
        host_(std::move(host)),
        kademlia_(std::move(kademlia)),
        self_peer_info_(std::move(self_peer_info)),
        sought_key_(std::move(sought_key)),
        nearest_peer_infos_(std::move(nearest_peer_infos)),
        handler_(std::move(handler)),
        log_("kad", "FindProvidersExecutor", ++instance_number) {
    std::for_each(
        nearest_peer_infos_.begin(), nearest_peer_infos_.end(),
        [this](auto &peer_info) { queue_.emplace(peer_info, sought_key_); });
  }

  outcome::result<void> FindProvidersExecutor::start() {
    if (started_) {
      return Error::IN_PROGRESS;
    }
    if (done_) {
      return Error::FULFILLED;
    }
    started_ = true;

    serialized_request_ = std::make_shared<std::vector<uint8_t>>();

    Message request = createGetProvidersRequest(sought_key_, self_peer_info_);
    if (!request.serialize(*serialized_request_)) {
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    spawn();

    return outcome::success();
  };

  void FindProvidersExecutor::spawn() {
    while (not queue_.empty()
           and requests_in_progress_ < max_concurent_requests_) {
      auto &peer_info = *queue_.top();
      queue_.pop();

      ++requests_in_progress_;

      log_.debug("connecting to {}, all: {}", peer_info.id.toBase58(),
                 requests_in_progress_);

      host_->newStream(
          peer_info, config_.protocolId,
          [self = shared_from_this()](auto &&stream_res) {
            self->onConnected(std::forward<decltype(stream_res)>(stream_res));
          });
    }

    if (requests_in_progress_ == 0) {
      handler_(Error::VALUE_NOT_FOUND);
    }
  }

  void FindProvidersExecutor::onConnected(
      outcome::result<std::shared_ptr<connection::Stream>> stream_res) {
    if (not stream_res) {
      --requests_in_progress_;

      log_.warn("cannot connect to peer: {}; active {} req.",
                stream_res.error().message(), requests_in_progress_);

      spawn();
      return;
    }

    auto &stream = stream_res.value();
    assert(stream->remoteMultiaddr());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    log_.debug("connected to {}; active {} req.", addr, requests_in_progress_);

    auto session = kademlia_->openSession(stream);

    if (!session->write(serialized_request_, shared_from_this())) {
      --requests_in_progress_;

      log_.warn("write to {} failed; active {} req.", addr,
                requests_in_progress_);

      spawn();
      return;
    }
  }

  bool FindProvidersExecutor::match(const Message &msg) const {
    return
        // Check if message type is appropriate
        msg.type == Message::Type::kFindNode
        // Check if response is accorded to request
        && msg.key == sought_key_.data;
  }

  void FindProvidersExecutor::onResult(const std::shared_ptr<Session> &session,
                                       outcome::result<Message> res) {
    // Check if gotten some message
    if (not res) {
      --requests_in_progress_;

      log_.warn("Result was not gotten from peer: {}; active {} req.",
                res.error().message(), requests_in_progress_);

      spawn();
      return;
    }
    auto &msg = res.value();

    // Skip inappropriate messages
    if (not match(msg)) {
      BOOST_UNREACHABLE_RETURN(false);
    }

    log_.debug("Result was gotten from peer: {}; active {} req.",
               session->stream()->remotePeerId().value().toBase58(),
               requests_in_progress_);

    if (msg.record) {
    }

    // Append gotten peer to queue
    if (msg.closer_peers) {
      for (auto &peer : msg.closer_peers.value()) {
        if (peer.info.id == self_peer_info_->id) {
          continue;
        }
        if (peer.conn_status == Message::Connectedness::CAN_CONNECT) {
          if (nearest_peer_infos_.emplace(peer.info).second) {
            queue_.emplace(peer.info, sought_key_);
          }
        }
      }
    }

    spawn();
  }

}  // namespace libp2p::protocol::kademlia
