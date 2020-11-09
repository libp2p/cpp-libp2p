/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/get_value_executor.hpp>
#include <libp2p/protocol/kademlia/impl/put_value_executor.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  std::atomic_size_t GetValueExecutor::instance_number = 0;

  GetValueExecutor::GetValueExecutor(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<SessionHost> session_host,
      std::shared_ptr<PeerRouting> peer_routing,
      std::shared_ptr<ContentRoutingTable> content_routing_table,
      std::shared_ptr<Validator> validator,
      std::shared_ptr<ExecutorsFactory> executor_factory,
      ContentId sought_content_id,
      std::unordered_set<PeerInfo> nearest_peer_infos,
      FoundValueHandler handler)
      : config_(config),
        host_(std::move(host)),
        session_host_(std::move(session_host)),
        peer_routing_(std::move(peer_routing)),
        content_routing_table_(std::move(content_routing_table)),
        validator_(std::move(validator)),
        executor_factory_(std::move(executor_factory)),
        sought_content_id_(std::move(sought_content_id)),
        nearest_peer_infos_(
            std::move(reinterpret_cast<decltype(nearest_peer_infos_) &>(
                nearest_peer_infos))),
        handler_(std::move(handler)),
        log_("kad", "GetValueExecutor", ++instance_number) {
    BOOST_ASSERT(host_ != nullptr);
    BOOST_ASSERT(session_host_ != nullptr);
    BOOST_ASSERT(peer_routing_ != nullptr);
    BOOST_ASSERT(content_routing_table_ != nullptr);
    BOOST_ASSERT(executor_factory_ != nullptr);
    std::for_each(nearest_peer_infos_.begin(), nearest_peer_infos_.end(),
                  [this](auto &peer_info) {
                    queue_.emplace(peer_info, sought_content_id_);
                  });
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

    Message request =
        createGetValueRequest(sought_content_id_, std::move(self_announce));
    if (!request.serialize(*serialized_request_)) {
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    started_ = true;

    log_.debug("started");

    spawn();
    return outcome::success();
  };

  void GetValueExecutor::spawn() {
    while (started_ and not done_ and not queue_.empty()
           and requests_in_progress_ < config_.requestConcurency) {
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

  void GetValueExecutor::onConnected(
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

    if (!session->write(serialized_request_, shared_from_this())) {
      --requests_in_progress_;

      log_.warn("write to {} failed; active {} req.", addr,
                requests_in_progress_);

      spawn();
      return;
    }
  }

  bool GetValueExecutor::match(const Message &msg) const {
    return
        // Check if message type is appropriate
        msg.type == Message::Type::kGetValue
        // Check if response is accorded to request
        && msg.key == sought_content_id_.data;
  }

  void GetValueExecutor::onResult(const std::shared_ptr<Session> &session,
                                  outcome::result<Message> msg_res) {
    gsl::final_action respawn([this] {
      --requests_in_progress_;
      spawn();
    });

    // Check if gotten some message
    if (not msg_res) {
      log_.warn("Result from {}: {}; active {} req.",
                session->stream()->remotePeerId().value().toBase58(),
                msg_res.error().message(), requests_in_progress_);
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

    // Append gotten peer to queue
    if (msg.closer_peers) {
      for (auto &peer : msg.closer_peers.value()) {
        // Add/Update peer info
        if (peer.conn_status != Message::Connectedness::CAN_NOT_CONNECT) {
          peer_routing_->addPeer(peer.info, false);
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
        if (nearest_peer_infos_.emplace(peer.info).second) {
          queue_.emplace(peer.info, sought_content_id_);
        }
      }
    }

    if (msg.record) {
      // TODO(xDimon): Validation here

      auto &value = msg.record.value().value;

      received_records_->insert({remote_peer_id, std::move(value)});

      if (received_records_->size() >= config_.valueLookupsQuorum) {
        // TODO(xDimon): Get best
        Value best;

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
            content_routing_table_->addProvider(sought_content_id_, peer);
          }
        }

        if (not addressees.empty()) {
          auto put_value_executor = executor_factory_->createPutValueExecutor(
              sought_content_id_, std::move(value), std::move(addressees));
          [[maybe_unused]] auto res = put_value_executor->start();
        }
      }
    }
  }
}  // namespace libp2p::protocol::kademlia
