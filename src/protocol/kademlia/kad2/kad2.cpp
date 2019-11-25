#include "kad2.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::kad2, Error, e) {
  using E = libp2p::kad2::Error;
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
    default:
      break;
  }
  return "unknown error";
}

namespace libp2p::kad2 {

  KadImpl::RequestHandler KadImpl::request_handlers_table[] = {
    &KadImpl::onPutValue,
    &KadImpl::onGetValue,
    &KadImpl::onAddProvider,
    &KadImpl::onGetProviders,
    &KadImpl::onFindNode,
    &KadImpl::onPing
  };

  KadImpl::KadImpl(HostAccessPtr host_access, RoutingTablePtr table,
          kad1::KademliaConfig config)
      : config_(config),
        host_(std::move(host_access)),
        table_(std::move(table))
  {}

  peer::Protocol KadImpl::getProtocolId() const  {
    return peer::Protocol(config_.protocolId);
  }

  void KadImpl::handle(StreamResult rstream) {
    if (not started_ or not is_server_) {
      return;
    }
    if (!rstream) {
      log_->info("KadImpl {}: incoming connection failed due to '{}'", (void*)this,
                 rstream.error().message());
      return;
    }

    log_->debug("KadImpl {}: incoming connection from '{}'", (void*)this,
               rstream.value()->remoteMultiaddr().value().getStringAddress());

    newServerSession(std::move(rstream.value()));
  }

  void KadImpl::newServerSession(std::shared_ptr<connection::Stream> stream) {
    connection::Stream *s = stream.get();
    assert(sessions_.find(s) == sessions_.end());
    auto session = std::make_shared<KadProtocolSession>(weak_from_this(),
                                                        std::move(stream));
    if (!session->read()) {
      s->reset();
      return;
    }
    session->state(reading_from_peer);
    sessions_[s] = { std::move(session), KadResponseHandler::Ptr{} };
  }

  void KadImpl::start(bool start_server) {
    if (start_server) {
      host_->startServer(shared_from_this());
      is_server_ = true;
    }

    new_channel_subscription_ =
      host_->getBus().getChannel<libp2p::network::event::OnNewConnectionChannel>().subscribe(
      [this](std::weak_ptr<libp2p::connection::CapableConnection> conn) {
        if (auto c = conn.lock()) {
          // adding outbound connections only
          if (c->isInitiator()) {
            log_->debug("KadImpl {}: new outbound connection", (void*)this);
            auto remote_peer_res = c->remotePeer();
            if (!remote_peer_res) {
              return;
            }
            auto remote_peer_addr_res = c->remoteMultiaddr();
            if (!remote_peer_addr_res) {
              return;
            }
            addPeer(peer::PeerInfo{
              std::move(remote_peer_res.value()), { std::move(remote_peer_addr_res.value()) }
              }, false);
          }
        }
      });

    started_ = true;
  }

  void KadImpl::addPeer(libp2p::peer::PeerInfo peer_info, bool permanent) {
    auto res = host_->getAddressRepository().upsertAddresses(
      peer_info.id,
      gsl::span(peer_info.addresses.data(), peer_info.addresses.size()),
      permanent ? peer::ttl::kPermanent : peer::ttl::kDay
    );
    if (res) {
      res = table_->update(peer_info.id);
    }
    std::string id_str = peer_info.id.toBase58();
    if (res) {
      log_->debug("KadImpl {}: successfully added peer to table: {}", (void*)this, id_str);
    } else {
      log_->debug("KadImpl {}: failed to add peer to table: {} : {}", (void*)this, id_str, res.error().message());
    }
  }

  KadImpl::Session* KadImpl::findSession(connection::Stream* from) {
    auto it = sessions_.find(from);
    if (it == sessions_.end()) {
      log_->warn("KadImpl {}: cannot find session by stream", (void*)this);
      return nullptr;
    }
    return &it->second;
  }

  void KadImpl::onMessage(connection::Stream *from, Message &&msg) {
    auto session = findSession(from);
    if (!session) return;

    if (session->response_handler) {
      // this is a client session

      auto peer_res = from->remotePeerId();
      assert(peer_res); //???? check

      if (msg.type != session->response_handler->expectedResponseType()) {
        session->response_handler->onResult(peer_res.value(), outcome::failure(Error::UNEXPECTED_MESSAGE_TYPE));
      } else {
        session->response_handler->onResult(peer_res.value(), std::move(msg));
      }

      closeSession(from);
    } else {
      // this is a server session

      log_->debug("KadImpl {}: request from '{}', type = {}", (void*)this,
                  from->remoteMultiaddr().value().getStringAddress(), msg.type);

      bool close_session =
        (msg.type >= Message::kTableSize) ||
        (not (this->*(request_handlers_table[msg.type]))(msg)) ||
        (not session->protocol_handler->write(msg));

      if (close_session) {
        closeSession(from);
      } else {
        session->protocol_handler->state(writing_to_peer);
      }
    }
  }

  void KadImpl::onCompleted(connection::Stream *from,
                   outcome::result<void> res)
  {
    auto session = findSession(from);
    if (!session) return;

    if (session->response_handler) {
      // this is a client session

      if (res.error() == Error::SUCCESS && session->protocol_handler->state() == writing_to_peer) {
        // request has been written, wait for response
        if (session->protocol_handler->read()) {
          session->protocol_handler->state(reading_from_peer);
          return;
        }
        res = outcome::failure(Error::STREAM_RESET);
      }

      auto peer_res = from->remotePeerId();
      assert(peer_res); //???? check

      session->response_handler->onResult(peer_res.value(), outcome::failure(res.error()));
      log_->debug("KadImpl {}: client session completed, total sessions: {}", (void*)this,sessions_.size()-1);

    } else {
      // this is a server session
      log_->debug("KadImpl {}: server session completed, total sessions: {}", (void*)this,sessions_.size()-1);
    }
    closeSession(from);
  }

  const kad1::KademliaConfig & KadImpl::config() {
    return config_;
  }

  void KadImpl::closeSession(connection::Stream *s) {
    auto it = sessions_.find(s);
    if (it != sessions_.end()) {
      it->second.protocol_handler->close();
      sessions_.erase(s);
    }
  }

  bool KadImpl::onPutValue(Message& msg) {
    log_->warn("KadImpl {}: {} NYI", (void*)this, __FUNCTION__);
    return false;
  }

  bool KadImpl::onGetValue(Message& msg) {
    log_->warn("KadImpl {}: {} NYI", (void*)this,__FUNCTION__);
    return false;
  }

  bool KadImpl::onAddProvider(Message& msg) {
    log_->warn("KadImpl {}: {} NYI", (void*)this,__FUNCTION__);
    return false;
  }

  bool KadImpl::onGetProviders(Message& msg) {
    log_->warn("KadImpl {}: {} NYI", (void*)this,__FUNCTION__);
    return false;
  }

  bool KadImpl::onFindNode(Message& msg) {
    log_->debug("KadImpl {}: {}", (void*)this,__FUNCTION__);

    if (msg.closer_peers) {
      for (auto& p : msg.closer_peers.value()) {
        if (p.conn_status == Message::Connectedness::CAN_CONNECT) {
          addPeer(std::move(p.info), false);
        }
      }
    }
    msg.closer_peers.reset();

    auto id_res = peer::PeerId::fromBytes(gsl::span(msg.key.data(), msg.key.size()));
    if (id_res) {
      auto ids = table_->getNearestPeers(kad1::NodeId(id_res.value()), 20);
      msg.closer_peers = Message::Peers{};
      auto& v = msg.closer_peers.value();
      v.reserve(config_.ALPHA);
      for (const auto& p : ids) {
        auto info = host_->getPeerInfo(p);
        if (info.addresses.empty()) {
          continue;
        }
        auto connectedness = host_->peerConnectedness(info);
        v.push_back({std::move(info), connectedness});
        if (v.size() >= config_.ALPHA) {
          break;
        }
      }
    }

    return true;
  }

  bool KadImpl::onPing(Message& msg) {
    log_->debug("KadImpl {}: {}", (void*)this,__FUNCTION__);

    if (msg.closer_peers) {
      for (auto& p : msg.closer_peers.value()) {
        if (p.conn_status == Message::Connectedness::CAN_CONNECT) {
          addPeer(std::move(p.info), false);
        }
      }
    }

    msg.clear();
    return true;
  }

  class FindPeerBatchHandler : public KadResponseHandler {
   public:
    FindPeerBatchHandler(peer::PeerId self, peer::PeerId key, Kad::FindPeerQueryResultFunc f, Kad& kad)
      : self_(std::move(self)), key_(std::move(key)), callback_(std::move(f)), kad_(kad)
    {}

    void wait_for(const peer::PeerId& id) {
      waiting_for_.insert(id);
    }

   private:
    Message::Type expectedResponseType() override { return Message::kFindNode; }

    void onResult(const peer::PeerId& from, outcome::result<Message> result) override {
      log_->debug("{} : findPeer: {} waiting for {} responses", (void*)&kad_, from.toBase58(), waiting_for_.size());
      waiting_for_.erase(from);
      if (!result) {
        log_->warn("{}: findPeer request to {} failed: {}", (void*)&kad_, from.toBase58(), result.error().message());
      } else {
        Message& msg = result.value();
        size_t n = 0;
        if (msg.closer_peers) {
          n = msg.closer_peers.value().size();
          for (auto& p : msg.closer_peers.value()) {
            if (p.info.id == self_) {
              --n;
              continue;
            }
            if (p.conn_status != Message::Connectedness::CAN_NOT_CONNECT &&
              p.conn_status != Message::Connectedness::NOT_CONNECTED) {
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
        log_->debug("{} : findPeer: {} returned {} records, waiting for {} responses", (void*)&kad_, from.toBase58(), n, waiting_for_.size());
      }
      if (callback_ && (result_.success || waiting_for_.empty())) {
        callback_(key_, std::move(result_));
        callback_ = Kad::FindPeerQueryResultFunc();
      } else {
        log_->debug("{} : ...", (void*)&kad_);
      }
    }

    peer::PeerId self_;
    peer::PeerId key_;
    Kad::FindPeerQueryResultFunc callback_;
    Kad& kad_;
    Kad::FindPeerQueryResult result_;
    std::unordered_set<peer::PeerId> waiting_for_;
    common::Logger log_ = common::createLogger("kad");
  };

  bool KadImpl::findPeer(const peer::PeerId& peer, Kad::FindPeerQueryResultFunc f) {
    log_->debug("KadImpl {}: new {} request", (void*)this,__FUNCTION__);

    auto pi = host_->getPeerInfo(peer);
    if (!pi.addresses.empty()) {
      // found locally, ids are sorted by distance
      Kad::FindPeerQueryResult result;
      result.success = true;
      result.peer = std::move(pi);
      // TODO result.closer_peers needed here? probably not
      // TODO decouple f() call in async manner!
      f(peer, std::move(result));
      log_->info("KadImpl {}: {} found locally from host!", (void*)this, peer.toBase58());
      return true;
    }

    auto ids = table_->getNearestPeers(kad1::NodeId(peer), 20);
    if (ids.empty()) {
      log_->info("KadImpl {}: {} : no peers", (void*)this, peer.toBase58());
      return false;
    }

    //  TODO check comparator
    auto it = std::find(ids.begin(), ids.end(), peer);
    //if (ids[0] == peer) {
    if (it != ids.end()) {
      // found locally, ids are sorted by distance
      Kad::FindPeerQueryResult result;
      result.success = true;
      result.peer = host_->getPeerInfo(peer);
      // TODO result.closer_peers needed here? probably not
      // TODO decouple f() call in async manner!
      f(peer, std::move(result));
      log_->info("KadImpl {}: {} found locally", (void*)this, peer.toBase58());
      return true;
    }

    std::unordered_set<peer::PeerInfo> v;

    for (const auto& p : ids) {
      auto info = host_->getPeerInfo(p);
      if (info.addresses.empty()) {
        continue;
      }
      auto connectedness = host_->peerConnectedness(info);
      if (connectedness == Message::Connectedness::CONNECTED
        || connectedness == Message::Connectedness::CAN_CONNECT)
      {
        v.insert(std::move(info));
        if (v.size() >= config_.ALPHA) {
          break;
        }
      }
    }

    if (v.empty()) {
      log_->info("KadImpl {}: {} : no peers to connect to", (void*)this, peer.toBase58());
      return false;
    }

    return findPeer(peer, v, std::move(f));
  }

  bool KadImpl::findPeer(const peer::PeerId& peer, const std::unordered_set<peer::PeerInfo>& closer_peers,
    Kad::FindPeerQueryResultFunc f)
  {
    std::optional<peer::PeerInfo> self_announce;
    peer::PeerInfo self = host_->thisPeerInfo();
    if (is_server_) {
      self_announce = self;
    }

    Message request = createFindNodeRequest(peer, std::move(self_announce));

    KadProtocolSession::Buffer buffer = std::make_shared<std::vector<uint8_t>>();
    if (!request.serialize(*buffer)) {
      log_->error("KadImpl {}: serialize error", (void*)this);
      return false;
    }

    auto handler = std::make_shared<FindPeerBatchHandler>(self.id, peer, std::move(f), *this);

    for (const auto& pi : closer_peers) {
      connect(pi, handler, buffer);
      handler->wait_for(pi.id);
    }

    return true;
  }

  void KadImpl::connect(
    const peer::PeerInfo& pi, const std::shared_ptr<KadResponseHandler>& handler,
    const KadProtocolSession::Buffer& request
  ) {
    uint64_t id = ++connecting_sessions_counter_;

    log_->debug("KadImpl {}: connecting to {}, {}", (void*)this, pi.id.toBase58(), handler.use_count());

    host_->dial(
      pi, getProtocolId(),
      [wptr=weak_from_this(), this, id, r = request, peerId=pi.id](auto &&stream_res) {
        auto self = wptr.lock();
        if (self) {
          onConnected(id, peerId, std::forward<decltype(stream_res)>(stream_res), r);
        }
      }
    );
    connecting_sessions_[id] = handler;
  }

  void KadImpl::onConnected(
    uint64_t id, const peer::PeerId& peerId, outcome::result<std::shared_ptr<connection::Stream>> stream_res,
    KadProtocolSession::Buffer request
  ) {
    auto it = connecting_sessions_.find(id);
    if (it == connecting_sessions_.end()) {
      log_->warn("KadImpl {}: cannot find connecting session {}", (void*)this, id);
      return;
    }
    auto handler = it->second;
    connecting_sessions_.erase(it);

    if (!stream_res) {
      log_->warn("KadImpl {}: cannot connect to server: {}", (void*)this, stream_res.error().message());
      handler->onResult(peerId, outcome::failure(stream_res.error()));
      return;
    }

    auto stream = stream_res.value().get();
    assert(stream->remoteMultiaddr());
    assert(sessions_.find(stream) == sessions_.end());

    std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    log_->debug("KadImpl {}: connected to {}, ({} - {})", (void*)this, addr, stream_res.value().use_count(), connecting_sessions_.size());


    auto protocol_session = std::make_shared<KadProtocolSession>(weak_from_this(), std::move(stream_res.value()));
    if (!protocol_session->write(std::move(request))) {
      log_->warn("KadImpl {}: write to {} failed", (void*)this, addr);
      assert(stream->remotePeerId());
      handler->onResult(peerId, Error::STREAM_RESET);
      return;
    }
    protocol_session->state(writing_to_peer);

    sessions_[stream] = Session { std::move(protocol_session), std::move(handler) };
    log_->debug("KadImpl {}: total sessions: {}", (void*)this,sessions_.size());
  }

  std::shared_ptr<Kad> createDefaultKadImpl(const std::shared_ptr<libp2p::Host>& h, RoutingTablePtr rt) {
    return  std::make_shared<KadImpl>(std::make_unique<HostAccessImpl>(*h), std::move(rt));
  }

} //namespace

