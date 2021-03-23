/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_CONNECTIVITY_HPP
#define LIBP2P_PROTOCOL_GOSSIP_CONNECTIVITY_HPP

#include <unordered_map>
#include <map>

#include <libp2p/host/host.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

#include "peer_set.hpp"
#include "stream.hpp"

namespace libp2p::protocol::gossip {

  class MessageReceiver;

  /// Part of GossipCore: Protocol server and network connections manager
  class Connectivity : public protocol::BaseProtocol,
                       public std::enable_shared_from_this<Connectivity> {
   public:
    Connectivity(const Connectivity &) = delete;
    Connectivity &operator=(const Connectivity &) = delete;
    Connectivity(Connectivity &&) = delete;
    Connectivity &operator=(Connectivity &&) = delete;

    using ConnectionStatusFeedback =
        std::function<void(bool connected, const PeerContextPtr &ctx)>;

    Connectivity(Config config, std::shared_ptr<Scheduler> scheduler,
                 std::shared_ptr<Host> host,
                 std::shared_ptr<MessageReceiver> msg_receiver,
                 ConnectionStatusFeedback on_connected);

    ~Connectivity() override;

    void start();

    /// Stops all client-server operations
    void stop();

    /// Adds bootstrap peer to the set of connectable peers
    void addBootstrapPeer(peer::PeerId id,
                          boost::optional<multi::Multiaddress> address);

    /// Add peer to writable set, actual writes occur on flush() (piggybacking)
    /// The idea behind writable set and flush() is a compromise between
    /// latency and message rate
    void peerIsWritable(const PeerContextPtr &ctx, bool low_latency);

    /// Flushes all pending writes for peers in writable set
    void flush();

    /// Performs periodic tasks and broadcasts heartbeat message to
    /// all connected peers. The changes are subscribe/unsubscribe events
    void onHeartbeat(const std::map<TopicId, bool> &local_changes);

    /// Returns connected peers
    const PeerSet &getConnectedPeers() const;

   private:
    using BannedPeers = std::set<std::pair<Time, PeerContextPtr>>;

    /// BaseProtocol override
    peer::Protocol getProtocolId() const override;

    /// BaseProtocol override, on new inbound stream
    void handle(StreamResult rstream) override;

    /// Tries to connect to peer
    void dial(const PeerContextPtr &peer, bool connection_must_exist);

    /// Attaches new stream to peer context
    void onNewStream(std::shared_ptr<connection::Stream> stream,
                     bool is_outbound);

    /// Async feedback from streams
    void onStreamEvent(const PeerContextPtr &from,
                       outcome::result<Success> event);

    /// Bans peer from outbound candidates list for configured time interval
    void ban(const PeerContextPtr &ctx);

    /// Unbans peer
    void unban(const PeerContextPtr &peer);

    /// Unbans peer
    void unban(BannedPeers::iterator it);

    /// Flushes outgoing messages into wire for a given peer, if connected
    void flush(const PeerContextPtr &ctx) const;

    const Config config_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<MessageReceiver> msg_receiver_;
    ConnectionStatusFeedback connected_cb_;
    Stream::Feedback on_stream_event_;
    bool started_ = false;

    /// All known peers
    PeerSet all_peers_;

    /// Peers can be dialed to
    PeerSet connectable_peers_;

    /// Peers temporary banned due to connectivity problems,
    /// will become connectable after certain interval
    BannedPeers banned_peers_expiration_;

    /// Writable peers
    PeerSet connected_peers_;

    /// Peers with pending write operation before the next heartbeat
    PeerSet writable_peers_low_latency_;

    /// Peers to be flushed on next heartbeat
    PeerSet writable_peers_on_heartbeat_;

    /// Logger
    log::SubLogger log_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_CONNECTIVITY_HPP
