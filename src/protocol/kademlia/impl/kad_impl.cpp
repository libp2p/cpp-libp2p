/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/types.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/protocol/kademlia/impl/kad_server.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol::kademlia, Error, e) {
  using E = libp2p::protocol::kademlia::Error;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::NO_PEERS:
      return "no peers found";
    case E::MESSAGE_PARSE_ERROR:
      return "message deserialize error";
    case E::MESSAGE_SERIALIZE_ERROR:
      return "message serialize error";
    case E::UNEXPECTED_MESSAGE_TYPE:
      return "unexpected_message_type";
    case E::STREAM_RESET:
      return "stream reset";
    case E::VALUE_NOT_FOUND:
      return "value not found";
    case E::CONTENT_VALIDATION_FAILED:
      return "content validation failed";
    case E::TIMEOUT:
      return "operation timed out";
    default:
      break;
  }
  return "unknown error";
}

namespace libp2p::protocol::kademlia {

  KadImpl::KadImpl(std::shared_ptr<Host> host,
                   std::shared_ptr<Scheduler> scheduler,
                   std::shared_ptr<RoutingTable> table,
                   std::unique_ptr<ValueStoreBackend> storage,
                   KademliaConfig config)
      : config_(config),
        protocol_(config_.protocolId),
        host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        table_(std::move(table)),
        local_store_(std::make_unique<LocalValueStore>(*this)),
        providers_store_(*scheduler_, scheduler::toTicks(config_.max_record_age))
        {}

  KadImpl::~KadImpl() = default;

  void KadImpl::start(bool start_server) {
    if (start_server) {
      server_ = std::make_unique<KadServer>(*host_, *this);
    }

    new_channel_subscription_ =
        host_->getBus()
            .getChannel<libp2p::network::event::OnNewConnectionChannel>()
            .subscribe(
                [this](
                    // NOLINTNEXTLINE
                    std::weak_ptr<libp2p::connection::CapableConnection> conn) {
                  if (auto c = conn.lock()) {
                    // adding outbound connections only
                    if (c->isInitiator()) {
                      log_->debug("KadImpl {}: new outbound connection",
                                  (void *)this);
                      auto remote_peer_res = c->remotePeer();
                      if (!remote_peer_res) {
                        return;
                      }
                      auto remote_peer_addr_res = c->remoteMultiaddr();
                      if (!remote_peer_addr_res) {
                        return;
                      }
                      addPeer(
                          peer::PeerInfo{
                              std::move(remote_peer_res.value()),
                              {std::move(remote_peer_addr_res.value())}},
                          false);
                    }
                  }
                });

    started_ = true;
  }

  void KadImpl::addPeer(libp2p::peer::PeerInfo peer_info, bool permanent) {
    auto res = host_->getPeerRepository().getAddressRepository().upsertAddresses(
        peer_info.id,
        gsl::span(peer_info.addresses.data(), peer_info.addresses.size()),
        permanent ? peer::ttl::kPermanent : peer::ttl::kDay);
    if (res) {
      res = table_->update(peer_info.id);
    }
    std::string id_str = peer_info.id.toBase58();
    if (res) {
      log_->debug("KadImpl {}: successfully added peer to table: {}",
                  (void *)this, id_str);
    } else {
      log_->debug("KadImpl {}: failed to add peer to table: {} : {}",
                  (void *)this, id_str, res.error().message());
    }
  }

  void KadImpl::getNearestPeers(const NodeId& id, PeerIdVec& out) {
    out = table_->getNearestPeers(id, config_.closer_peers_count * 2);
  }

  KadImpl::Session *KadImpl::findSession(connection::Stream *from) {
    auto it = sessions_.find(from);
    if (it == sessions_.end()) {
      log_->warn("KadImpl {}: cannot find session by stream", (void *)this);
      return nullptr;
    }
    return &it->second;
  }

  void KadImpl::onMessage(connection::Stream *from, Message &&msg) {
    auto session = findSession(from);
    if (session == nullptr) {
      return;
    }

    auto peer_res = from->remotePeerId();
    assert(peer_res);  //???? check

    if (msg.type != session->response_handler->expectedResponseType()) {
      session->response_handler->onResult(
          peer_res.value(), outcome::failure(Error::UNEXPECTED_MESSAGE_TYPE));
    } else {
      session->response_handler->onResult(peer_res.value(), std::move(msg));
    }

    closeSession(from);
  }

  void KadImpl::onCompleted(connection::Stream *from,
                            outcome::result<void> res) {
    auto session = findSession(from);
    if (session == nullptr) {
      return;
    }

    if (session->response_handler->needResponse()) {
      if (res.error() == Error::SUCCESS
          && session->protocol_handler->state() == writing_to_peer) {
        // request has been written, wait for response
        if (session->protocol_handler->read()) {
          session->protocol_handler->state(reading_from_peer);
          return;
        }
        res = outcome::failure(Error::STREAM_RESET);
      }

      auto peer_res = from->remotePeerId();
      assert(peer_res);

      session->response_handler->onResult(peer_res.value(),
                                          outcome::failure(res.error()));
    }

    log_->debug("KadImpl {}: client session completed, total sessions: {}",
                (void *)this, sessions_.size() - 1);

    closeSession(from);
  }

  void KadImpl::closeSession(connection::Stream *s) {
    auto it = sessions_.find(s);
    if (it != sessions_.end()) {
      it->second.protocol_handler->close();
      sessions_.erase(s);
    }
  }

  class FindPeerBatchHandler : public KadResponseHandler {
   public:
    FindPeerBatchHandler(peer::PeerId self, peer::PeerId key,
                         Kad::FindPeerQueryResultFunc f, Kad &kad)
        : self_(std::move(self)),
          key_(std::move(key)),
          callback_(std::move(f)),
          kad_(kad) {}

    void wait_for(const peer::PeerId &id) {
      waiting_for_.insert(id);
    }

   private:
    Message::Type expectedResponseType() override {
      return Message::kFindNode;
    }

    bool needResponse() override {
      return true;
    }

    void onResult(const peer::PeerId &from,
                  outcome::result<Message> result) override {
      log_->debug("{} : findPeer: {} waiting for {} responses", (void *)&kad_,
                  from.toBase58(), waiting_for_.size());
      waiting_for_.erase(from);
      if (!result) {
        log_->warn("{}: findPeer request to {} failed: {}", (void *)&kad_,
                   from.toBase58(), result.error().message());
      } else {
        Message &msg = result.value();
        size_t n = 0;
        if (msg.closer_peers) {
          n = msg.closer_peers.value().size();
          for (auto &p : msg.closer_peers.value()) {
            if (p.info.id == self_) {
              --n;
              continue;
            }
            if (p.conn_status != Message::Connectedness::CAN_NOT_CONNECT
                && p.conn_status != Message::Connectedness::NOT_CONNECTED) {
              if (callback_) {
                if (p.info.id == key_) {
                  result_.success = true;
                  result_.peer = p.info;
                }
                result_.closer_peers.insert(p.info);
              }
              kad_.addPeer(std::move(p.info), false);
            }
          }
        }
        log_->debug(
            "{} : findPeer: {} returned {} records, waiting for {} responses",
            (void *)&kad_, from.toBase58(), n, waiting_for_.size());
      }
      if (callback_ && (result_.success || waiting_for_.empty())) {
        callback_(key_, std::move(result_));
        callback_ = Kad::FindPeerQueryResultFunc();
      } else {
        log_->debug("{} : ...", (void *)&kad_);
      }
    }

    peer::PeerId self_;
    peer::PeerId key_;
    Kad::FindPeerQueryResultFunc callback_;
    Kad &kad_;
    Kad::FindPeerQueryResult result_;
    std::unordered_set<peer::PeerId> waiting_for_;
    common::Logger log_ = common::createLogger("kad");
  };

  bool KadImpl::findPeer(const peer::PeerId &peer,
                         Kad::FindPeerQueryResultFunc f) {
    log_->debug("KadImpl {}: new {} request", (void *)this, __FUNCTION__);

    auto pi = host_->getPeerRepository().getPeerInfo(peer);
    if (!pi.addresses.empty()) {
      // found locally, ids are sorted by distance
      Kad::FindPeerQueryResult result;
      result.success = true;
      result.peer = std::move(pi);
      // TODO(artem): result.closer_peers needed here? probably not

      scheduler_->schedule(
          [f=std::move(f), p=peer, r=std::move(result)] { f(p, r); }
      ).detach();

      log_->info("KadImpl {}: {} found locally from host!", (void *)this,
                 peer.toBase58());
      return true;
    }

    auto ids = table_->getNearestPeers(NodeId(peer), 20);
    if (ids.empty()) {
      log_->info("KadImpl {}: {} : no peers", (void *)this, peer.toBase58());
      return false;
    }

    //  TODO(artem): check comparator
    auto it = std::find(ids.begin(), ids.end(), peer);
    // if (ids[0] == peer) {
    if (it != ids.end()) {
      // found locally, ids are sorted by distance
      Kad::FindPeerQueryResult result;
      result.success = true;
      result.peer = host_->getPeerRepository().getPeerInfo(peer);
      // TODO(artem): result.closer_peers needed here? probably not

      scheduler_->schedule(
          [f=std::move(f), p=peer, r=std::move(result)] { f(p, r); }
      ).detach();

      log_->info("KadImpl {}: {} found locally", (void *)this, peer.toBase58());
      return true;
    }

    std::unordered_set<peer::PeerInfo> v;

    for (const auto &p : ids) {
      auto info = host_->getPeerRepository().getPeerInfo(p);
      if (info.addresses.empty()) {
        continue;
      }
      auto connectedness =
          host_->getNetwork().getConnectionManager().connectedness(info);
      if (connectedness == Message::Connectedness::CONNECTED
          || connectedness == Message::Connectedness::CAN_CONNECT) {
        v.insert(std::move(info));
        if (v.size() >= config_.ALPHA) {
          break;
        }
      }
    }

    if (v.empty()) {
      log_->info("KadImpl {}: {} : no peers to connect to", (void *)this,
                 peer.toBase58());
      return false;
    }

    return findPeer(peer, v, std::move(f));
  }

  bool KadImpl::findPeer(const peer::PeerId &peer,
                         const std::unordered_set<peer::PeerInfo> &closer_peers,
                         Kad::FindPeerQueryResultFunc f) {
    std::optional<peer::PeerInfo> self_announce;
    peer::PeerInfo self = host_->getPeerInfo();
    if (server_) {
      self_announce = self;
    }

    Message request = createFindNodeRequest(peer, std::move(self_announce));

    KadProtocolSession::Buffer buffer =
        std::make_shared<std::vector<uint8_t>>();
    if (!request.serialize(*buffer)) {
      log_->error("KadImpl {}: serialize error", (void *)this);
      return false;
    }

    auto handler = std::make_shared<FindPeerBatchHandler>(self.id, peer,
                                                          std::move(f), *this);

    for (const auto &pi : closer_peers) {
      connect(pi, handler, buffer);
      handler->wait_for(pi.id);
    }

    return true;
  }

  void KadImpl::putValue(const ContentAddress& key, Value value, PutValueResultFunc f) {
    // puts value into local store and then it initiates broadcast process

    auto res = local_store_->putValue(key, std::move(value));

    // TODO(artem): subscriptions instead of coarse callback API

    scheduler_->schedule(
        [f=std::move(f), r=res] { f(r); }
    ).detach();
  }

  class GetValueBatchHandler : public KadResponseHandler {
   public:
    GetValueBatchHandler(ContentAddress key,
                         Kad::GetValueResultFunc f, KadImpl &kad)
        : key_(std::move(key)),
          callback_(std::move(f)),
          kad_(kad) {}

    void wait_for(const peer::PeerId &id) {
      waiting_for_.insert(id);
    }

   private:
    Message::Type expectedResponseType() override {
      return Message::kGetValue;
    }

    bool needResponse() override {
      return true;
    }

    void onResult(const peer::PeerId &from,
                  outcome::result<Message> result) override {
      log_->debug("{} : getValue: {} waiting for {} responses", (void *)&kad_,
                  from.toBase58(), waiting_for_.size());
      waiting_for_.erase(from);
      if (!result) {
        log_->warn("{}: getValue request to {} failed: {}", (void *)&kad_,
                   from.toBase58(), result.error().message());
      } else {
        Message &msg = result.value();

        log_->debug(
            "{} : getValue: response from {}, waiting for {} responses",
            (void *)&kad_, from.toBase58(),
            waiting_for_.size());

        if (msg.provider_peers) {
          for (auto &p : msg.provider_peers.value()) {
            if (p.conn_status != Message::Connectedness::CAN_NOT_CONNECT
                && p.conn_status != Message::Connectedness::NOT_CONNECTED) {
              kad_.getContentProvidersStore().addProvider(key_, p.info.id);
              kad_.addPeer(std::move(p.info), false);
            }
          }
        }

        if (msg.record) {
          // TODO(artem): validation and quorum

          auto res = kad_.getLocalValueStore().putValue(
              key_, msg.record.value().value);

          if (callback_ && res) {
            callback_(std::move(msg.record.value().value));
            callback_ = Kad::GetValueResultFunc();
          }
        }
      }

      if (callback_ && waiting_for_.empty()) {
        callback_(Error::VALUE_NOT_FOUND);
        callback_ = Kad::GetValueResultFunc();
      } else {
        log_->debug("{} : ...", (void *)&kad_);
      }
    }

    ContentAddress key_;
    Kad::GetValueResultFunc callback_;
    KadImpl &kad_;

    // TODO(artem): quorum and validators!

    std::unordered_set<peer::PeerId> waiting_for_;
    common::Logger log_ = common::createLogger("kad");
  };

  void KadImpl::getValue(const ContentAddress& key, GetValueResultFunc f) {
    LocalValueStore::AbsTime ts;
    auto res = local_store_->getValue(key, ts);
    if (res) {
      // TODO(artem): subscriptions instead of coarse callback API
      scheduler_->schedule(
          [f=std::move(f), r=res] { f(r); }
      ).detach();

      return;
    }

    PeerIdVec peers;
    providers_store_.getProvidersFor(key, peers);

    if (peers.size() < config_.get_many_records_count) {
      PeerIdVec closer_peers;
      getNearestPeers(NodeId(key), closer_peers);

      for (auto& p : closer_peers) {
        if (std::find(peers.begin(), peers.end(), p) == peers.end()) {
          peers.push_back(p);
          if (peers.size() == config_.get_many_records_count) {
            break;
          }
        }
      }
    }

    if (peers.empty()) {
      scheduler_->schedule(
          [f=std::move(f)] { f(Error::NO_PEERS); }
      ).detach();

      return;
    }

    Message request = createGetValueRequest(key);

    KadProtocolSession::Buffer buffer =
        std::make_shared<std::vector<uint8_t>>();
    if (!request.serialize(*buffer)) {
      log_->error("KadImpl {}: serialize error", (void *)this);

      scheduler_->schedule(
          [f=std::move(f)] { f(Error::MESSAGE_SERIALIZE_ERROR); }
      ).detach();

      return;
    }

    auto handler = std::make_shared<GetValueBatchHandler>(
        key, std::move(f), *this);

    for (const auto &p : peers) {
      auto peer_info = host_->getPeerRepository().getPeerInfo(p);
      if (peer_info.addresses.empty()) {
        continue;
      }
      connect(peer_info, handler, buffer);
      handler->wait_for(p);
    }
  }

  class BroadcastHandler : public KadResponseHandler {
   public:
    explicit BroadcastHandler(Kad &kad) : kad_(kad) {}

   private:
    Message::Type expectedResponseType() override {
      return Message::kPing;
    }

    bool needResponse() override {
      return false;
    }

    void onResult(const peer::PeerId &from, outcome::result<Message>) override {
      log_->warn("{}: unexpected response to broadcast request from {}",
          (void *)&kad_,
          from.toBase58());
    }

    Kad &kad_;
    common::Logger log_ = common::createLogger("kad");
  };

  void KadImpl::broadcastThisProvider(const ContentAddress& key) {
    PeerIdVec closer_peers;
    getNearestPeers(NodeId(key), closer_peers);

    if (closer_peers.empty()) {
      // TODO(artem): maybe reschedule this broadcast to a shorter interval
      // when peer table probably becomes not empty
      return;
    }

    Message request = createAddProviderRequest(host_->getPeerInfo(), key);

    KadProtocolSession::Buffer buffer =
        std::make_shared<std::vector<uint8_t>>();
    if (!request.serialize(*buffer)) {
      log_->error("KadImpl {}: serialize error", (void *)this);
      return;
    }

    auto handler = std::make_shared<BroadcastHandler>(*this);

    // TODO(artem): make more flexible config
    // depending on routing table maturity etc.
    auto max_recepients = config_.closer_peers_count;
    size_t n = 0;

    for (const auto &p : closer_peers) {
      auto peer_info = host_->getPeerRepository().getPeerInfo(p);
      if (peer_info.addresses.empty()) {
        continue;
      }
      connect(peer_info, handler, buffer);
      if (++n > max_recepients) {
        break;
      }
    }
  }

  void KadImpl::connect(const peer::PeerInfo &pi,
                        const std::shared_ptr<KadResponseHandler> &handler,
                        const KadProtocolSession::Buffer &request) {
    uint64_t id = ++connecting_sessions_counter_;

    log_->debug("KadImpl {}: connecting to {}, {}", (void *)this,
                pi.id.toBase58(), handler.use_count());

    host_->newStream(pi, protocol_,
                    [wptr = weak_from_this(), this, id, r = request,
                     peerId = pi.id](auto &&stream_res) {
                      auto self = wptr.lock();
                      if (self) {
                        onConnected(
                            id, peerId,
                            std::forward<decltype(stream_res)>(stream_res), r);
                      }
                    });
    connecting_sessions_[id] = handler;
  }

  void KadImpl::onConnected(
      uint64_t id, const peer::PeerId &peerId,
      outcome::result<std::shared_ptr<connection::Stream>> stream_res,
      KadProtocolSession::Buffer request) {
    auto it = connecting_sessions_.find(id);
    if (it == connecting_sessions_.end()) {
      log_->warn("KadImpl {}: cannot find connecting session {}", (void *)this,
                 id);
      return;
    }
    auto handler = it->second;
    connecting_sessions_.erase(it);

    if (!stream_res) {
      log_->warn("KadImpl {}: cannot connect to server: {}", (void *)this,
                 stream_res.error().message());
      handler->onResult(peerId, outcome::failure(stream_res.error()));
      return;
    }

    auto stream = stream_res.value().get();
    assert(stream->remoteMultiaddr());
    assert(sessions_.find(stream) == sessions_.end());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    log_->debug("KadImpl {}: connected to {}, ({} - {})", (void *)this, addr,
                stream_res.value().use_count(), connecting_sessions_.size());

    auto protocol_session = std::make_shared<KadProtocolSession>(
        weak_from_this(), std::move(stream_res.value()));
    if (!protocol_session->write(std::move(request))) {
      log_->warn("KadImpl {}: write to {} failed", (void *)this, addr);
      assert(stream->remotePeerId());
      handler->onResult(peerId, Error::STREAM_RESET);
      return;
    }
    protocol_session->state(writing_to_peer);

    sessions_[stream] =
        Session{std::move(protocol_session), std::move(handler)};
    log_->debug("KadImpl {}: total sessions: {}", (void *)this,
                sessions_.size());
  }

}  // namespace libp2p::protocol::kademlia
