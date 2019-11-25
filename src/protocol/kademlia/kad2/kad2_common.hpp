#pragma once

#include <libp2p/outcome/outcome-register.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/base_protocol.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/address_repository.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/protocol/kademlia/routing_table.hpp>
#include <libp2p/host/host.hpp>

namespace libp2p::kad2 {
  enum class Error {
    SUCCESS = 0,
    NO_PEERS = 1,
    MESSAGE_PARSE_ERROR = 2,
    MESSAGE_SERIALIZE_ERROR = 3,
    UNEXPECTED_MESSAGE_TYPE = 4,
    STREAM_RESET = 5
  };

  namespace kad1 = libp2p::protocol::kademlia;

  using kad1::Key;
  using kad1::Value;

  using RoutingTablePtr = std::shared_ptr<kad1::RoutingTable>;

  struct Message;

  class KadSessionHost {
   public:
    virtual ~KadSessionHost() = default;

    virtual void onMessage(connection::Stream* from, Message&& msg) = 0;

    virtual void onCompleted(connection::Stream* from, outcome::result<void> res) = 0;

    virtual const kad1::KademliaConfig& config() = 0;
  };

  class HostAccess {
   public:
    virtual ~HostAccess() = default;

    virtual void startServer(const std::shared_ptr<protocol::BaseProtocol>& handler) = 0;

    virtual event::Bus& getBus() = 0;

    virtual peer::PeerInfo getPeerInfo(const peer::PeerId &peer_id) = 0;

    virtual peer::PeerInfo thisPeerInfo() = 0;

    virtual peer::AddressRepository& getAddressRepository() = 0;

    virtual network::ConnectionManager::Connectedness peerConnectedness(const peer::PeerInfo &pi) = 0;

    using DialCallback = std::function<void(outcome::result<std::shared_ptr<connection::Stream>>)>;

    virtual void dial(const peer::PeerInfo &pi, const peer::Protocol& protocolId,
      const DialCallback& f) = 0;
  };

  using HostAccessPtr = std::unique_ptr<HostAccess>;

  /// Kademlia protocol server and client
  class Kad : public protocol::BaseProtocol {
   public:
    ~Kad() override = default;

    virtual void start(bool start_server) = 0;

    // permanent==true for bootstrap peers
    virtual void addPeer(peer::PeerInfo peer_info, bool permanent) = 0;

    struct FindPeerQueryResult {
      std::unordered_set<peer::PeerInfo> closer_peers{};
      boost::optional<peer::PeerInfo> peer{};
      bool success = false;
    };

    using FindPeerQueryResultFunc = std::function<void(const peer::PeerId& peer, FindPeerQueryResult)>;

    virtual bool findPeer(const peer::PeerId& peer, FindPeerQueryResultFunc f) = 0;

    virtual bool findPeer(const peer::PeerId& peer, const std::unordered_set<peer::PeerInfo>& closer_peers, FindPeerQueryResultFunc f) = 0;
  };

  std::shared_ptr<Kad> createDefaultKadImpl(const std::shared_ptr<libp2p::Host>& h, RoutingTablePtr rt);

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::kad2, Error);
