#pragma once
#include <libp2p/connection/stream.hpp>
#include "message_read_writer.hpp"
#include "kad2_common.hpp"
#include "kad_protocol_session.hpp"
#include "low_res_timer.hpp"

namespace libp2p::kad2 {


  class HostAccessImpl : public HostAccess {
   public:
    HostAccessImpl(Host& host) : host_(host)
    {}

    ~HostAccessImpl() override = default;

    void startServer(const std::shared_ptr<protocol::BaseProtocol>& handler) override {
      host_.setProtocolHandler(
        handler->getProtocolId(),
        [wptr = std::weak_ptr<protocol::BaseProtocol>(handler)](protocol::BaseProtocol::StreamResult rstream) {
          auto h = wptr.lock();
          if (h) {
            h->handle(std::move(rstream));
          }
        }
      );
    }

    event::Bus& getBus() override {
      return host_.getBus();
    }

    peer::PeerInfo getPeerInfo(const peer::PeerId &peer_id) override {
      return host_.getPeerRepository().getPeerInfo(peer_id);
    }

    peer::PeerInfo thisPeerInfo() override {
      return host_.getPeerInfo();
    }

    peer::AddressRepository& getAddressRepository() override {
      return host_.getPeerRepository().getAddressRepository();
    }

    network::ConnectionManager::Connectedness peerConnectedness(const peer::PeerInfo &pi) override {
      return host_.getNetwork().getConnectionManager().connectedness(pi);
    }

    void dial(const peer::PeerInfo &pi, const peer::Protocol& protocolId,
              const DialCallback& f) override {
      host_.newStream(pi, protocolId, f);
    }

   private:
    Host& host_;
  };

  class KadResponseHandler : public std::enable_shared_from_this<KadResponseHandler> {
   public:
    using Ptr = std::shared_ptr<KadResponseHandler>;
    virtual ~KadResponseHandler() = default;

    virtual Message::Type expectedResponseType() = 0;

    virtual void onResult(const peer::PeerId& from, outcome::result<Message> result) = 0;
  };


  class KadImpl : public Kad, public KadSessionHost, public std::enable_shared_from_this<KadImpl> {
   public:
    KadImpl(
      HostAccessPtr host_access, RoutingTablePtr table,
      kad1::KademliaConfig config = kad1::KademliaConfig{}
    );

    peer::Protocol getProtocolId() const override;

    void handle(StreamResult rstream) override;

    void start(bool start_server) override;

    void addPeer(peer::PeerInfo peer_info, bool permanent) override;

    bool findPeer(const peer::PeerId& peer, FindPeerQueryResultFunc f) override;

    bool findPeer(const peer::PeerId& peer, const std::unordered_set<peer::PeerInfo>& closer_peers, FindPeerQueryResultFunc f) override;

   private:
    enum SessionState {
      closed = KadProtocolSession::CLOSED_STATE,
      reading_from_peer, writing_to_peer };

    struct Session {
      KadProtocolSession::Ptr protocol_handler;

      // nullptr for server sessions
      KadResponseHandler::Ptr response_handler;
    };

    typedef bool (KadImpl::*RequestHandler)(Message& msg);

    Session* findSession(connection::Stream* from);

    void onMessage(connection::Stream* from, Message&& msg) override;

    void onCompleted(connection::Stream* from, outcome::result<void> res) override;

    const kad1::KademliaConfig& config() override;

    void newServerSession(std::shared_ptr<connection::Stream> stream);

    void closeSession(connection::Stream* s);

    void connect(const peer::PeerInfo& pi, const std::shared_ptr<KadResponseHandler>& handler,
                 const KadProtocolSession::Buffer& request);

    void onConnected(uint64_t id, const peer::PeerId& peerId, outcome::result<std::shared_ptr<connection::Stream>> stream_res,
      KadProtocolSession::Buffer request);

    // request handlers
    bool onPutValue(Message& msg);
    bool onGetValue(Message& msg);
    bool onAddProvider(Message& msg);
    bool onGetProviders(Message& msg);
    bool onFindNode(Message& msg);
    bool onPing(Message& msg);

    kad1::KademliaConfig config_;
    HostAccessPtr host_;
    RoutingTablePtr table_;
    common::Logger log_ = common::createLogger("kad");
    bool started_ = false;
    bool is_server_ = false;

    using Sessions = std::map<connection::Stream*, Session>;
    Sessions sessions_;

    using ConnectingSessions = std::map<uint64_t, KadResponseHandler::Ptr>;
    ConnectingSessions connecting_sessions_;
    uint64_t connecting_sessions_counter_ = 0;

    event::Handle new_channel_subscription_;

    static RequestHandler request_handlers_table[];
  };

 class KadSingleQueryClient : public KadSessionHost, public std::enable_shared_from_this<KadSingleQueryClient> {
  public:
   KadSingleQueryClient() = default;

   void dial(libp2p::Host& host, const peer::PeerInfo& connect_to, Message msg) {
     msg_ = std::move(msg);
     host.newStream(
       connect_to, peer::Protocol(config_.protocolId),
       [wptr=weak_from_this(), this](auto &&stream_res) {
         auto self = wptr.lock();
         if (self) {
           onConnected(std::forward<decltype(stream_res)>(stream_res));
         }
       }
     );
   }

  private:

   enum SessionState {
     closed = KadProtocolSession::CLOSED_STATE,
     reading_from_peer, writing_to_peer };

   void onConnected(outcome::result<std::shared_ptr<connection::Stream>> stream_res) {
     if (!stream_res) {
       log_->error("Cannot connect to server: {}", stream_res.error().message());
       return;
     }
     log_->debug("Connected to {}", stream_res.value()->remoteMultiaddr().value().getStringAddress());
     stream_ = stream_res.value().get();
     session_ = std::make_shared<KadProtocolSession>(weak_from_this(), std::move(stream_res.value()));
     if (!session_->write(msg_)) {
       close();
     } else {
       session_->state(writing_to_peer);
     }
   }

   void close() {
     if (stream_) {
       stream_->reset();
       session_->state(closed);
       session_.reset();
     }
   }

   void onMessage(connection::Stream* from, Message&& msg) override {
     if (from != stream_ || !session_) {
       log_->warn("streams ptr mismatch");
       return;
     }
     log_->debug("received message, type = {}", msg.type);
     close();
   }

   void onCompleted(connection::Stream* from, outcome::result<void> res) override {
     if (from != stream_ || !session_) {
       log_->warn("streams ptr mismatch");
       return;
     }
     if (session_->state() == reading_from_peer || res.error() != Error::SUCCESS) {
       log_->debug("session completed: {}", res.error().message());
       close();
     } else {
       session_->state(reading_from_peer);
       session_->read();
     }
   }

   const kad1::KademliaConfig& config() override {
     return config_;
   }

   kad1::KademliaConfig config_;
   Message msg_;
   connection::Stream* stream_ = nullptr;
   KadProtocolSession::Ptr session_;
   common::Logger log_ = common::createLogger("kad");
 };

} //namespace

