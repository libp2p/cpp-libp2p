/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/find_peer_executor.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  std::atomic_size_t FindPeerExecutor::instance_number = 0;

  FindPeerExecutor::FindPeerExecutor(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<SessionHost> session_host,
      std::shared_ptr<PeerRouting> peer_routing,
      boost::optional<PeerInfo> self_peer_info, PeerId sought_peer_id,
      std::unordered_set<PeerInfo> nearest_peer_infos,
      FoundPeerInfoHandler handler)
      : config_(config),
        host_(std::move(host)),
        session_host_(std::move(session_host)),
        peer_routing_(std::move(peer_routing)),
        self_peer_info_(std::move(self_peer_info)),
        sought_peer_id_(std::move(sought_peer_id)),
        sought_peer_id_as_content_id_(sought_peer_id_.toVector()),
        nearest_peer_infos_(
            std::move(reinterpret_cast<decltype(nearest_peer_infos_) &>(
                nearest_peer_infos))),
        handler_(std::move(handler)),
        log_("kad", "FindPeerExecutor", ++instance_number) {
    std::for_each(nearest_peer_infos_.begin(), nearest_peer_infos_.end(),
                  [this](auto &peer_info) {
                    queue_.emplace(peer_info,
                                   ContentId(sought_peer_id_.toVector()));
                  });
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

    serialized_request_ = std::make_shared<std::vector<uint8_t>>();

    Message request = createFindNodeRequest(sought_peer_id_, self_peer_info_);
    if (!request.serialize(*serialized_request_)) {
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    started_ = true;

    log_.debug("started");

    spawn();

    return outcome::success();
  };

  void FindPeerExecutor::spawn() {
    while (started_ and not done_ and not queue_.empty()
           and requests_in_progress_ < max_concurent_requests_) {
      auto &peer_info = *queue_.top();
      queue_.pop();

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
      handler_(Error::VALUE_NOT_FOUND);
    }
  }

  void FindPeerExecutor::onConnected(
      outcome::result<std::shared_ptr<connection::Stream>> stream_res) {
    if (not stream_res) {
      --requests_in_progress_;

      log_.debug("cannot connect to peer: {}; active {} req.",
                 stream_res.error().message(), requests_in_progress_);

      spawn();
      return;
    }

    auto &stream = stream_res.value();
    assert(stream->remoteMultiaddr());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    log_.debug("connected to {}; active {} req.", addr, requests_in_progress_);

    auto session = session_host_->openSession(stream);

    if (!session->write(serialized_request_, shared_from_this())) {
      --requests_in_progress_;

      log_.warn("write to {} failed; active {} req.", addr,
                requests_in_progress_);

      spawn();
      return;
    }
  }

  bool FindPeerExecutor::match(const Message &msg) const {
    return
        // Check if message type is appropriate
        msg.type == Message::Type::kFindNode
        // Check if response is accorded to request
        && msg.key == sought_peer_id_.toVector();
  }

  void FindPeerExecutor::onResult(const std::shared_ptr<Session> &session,
                                  outcome::result<Message> res) {
    gsl::final_action respawn([this] {
      --requests_in_progress_;
      spawn();
    });

    // Check if gotten some message
    if (not res) {
      log_.warn("Result from {}: {}; active {} req.",
                session->stream()->remotePeerId().value().toBase58(),
                res.error().message(), requests_in_progress_);
      return;
    }
    auto &msg = res.value();

    // Skip inappropriate messages
    if (not match(msg)) {
      BOOST_UNREACHABLE_RETURN();
    }

    auto remote_peer_id_res = session->stream()->remotePeerId();
    BOOST_ASSERT(remote_peer_id_res.has_value());
    auto &remote_peer_id = remote_peer_id_res.value();

    log_.debug("Result from {}: gotten; active {} req.",
               remote_peer_id.toBase58(), requests_in_progress_);

    // Append gotten peer to queue
    if (msg.closer_peers) {
      for (auto &peer : msg.closer_peers.value()) {
        // Add/Update peer info
        if (peer.conn_status != Message::Connectedness::CAN_NOT_CONNECT) {
          peer_routing_->addPeer(peer.info, false);
        }

        // Skip himself
        if (peer.info.id == self_peer_info_->id) {
          continue;
        }

        // Skip remote
        if (peer.info.id == remote_peer_id) {
          continue;
        }

        // New peer add to queue
        if (nearest_peer_infos_.emplace(peer.info).second) {
          queue_.emplace(peer.info, sought_peer_id_as_content_id_);
        }
      }
    }

    if (msg.record) {
      // TODO(xDimon): Need to implement selection of result by quorum

      done_ = true;
      log_.debug("done");
      handler_(PeerInfo{
          .id = sought_peer_id_,
          // TODO(xDimon): How to read bytes to vector of multiaddresses?
          .addresses = {}  // {msg.record->value}
      });
    }
  }

}  // namespace libp2p::protocol::kademlia
