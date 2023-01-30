/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_CONNECTIVITY_HPP
#define LIBP2P_PROTOCOL_GOSSIP_CONNECTIVITY_HPP

#include <map>
#include <unordered_map>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/sublogger.hpp>

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

    Connectivity(Config config, std::shared_ptr<basic::Scheduler> scheduler,
                 std::shared_ptr<Host> host,
                 std::shared_ptr<MessageReceiver> msg_receiver,
                 ConnectionStatusFeedback on_connected);

    ~Connectivity() override;

    void start();

    /// Stops all client-server operations
    void stop();

    /// Adds bootstrap peer to the set of connectable peers
    void addBootstrapPeer(const peer::PeerId &id,
                          const boost::optional<multi::Multiaddress> &address);

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
    peer::ProtocolName getProtocolId() const override;

    /// BaseProtocol override, on new inbound stream
    void handle(StreamAndProtocol stream) override;

    /// Tries to connect to peer
    void dial(const PeerContextPtr &peer);

    /// Tries to connect to peer over existing connection
    void dialOverExistingConnection(const PeerContextPtr &peer);

    /// Outbound stream result callback
    void onNewStream(const PeerContextPtr &ctx,
                     StreamAndProtocolOrError rstream);

    /// Async feedback from streams
    void onStreamEvent(const PeerContextPtr &from,
                       outcome::result<Success> event);

    /// Bans peer from outbound candidates list for configured time interval
    void banOrForget(const PeerContextPtr &ctx);

    /// Unbans peer
    void unban(const PeerContextPtr &peer);

    /// Unbans peer
    void unban(BannedPeers::iterator it);

    /// Flushes outgoing messages into wire for a given peer, if connected
    void flush(const PeerContextPtr &ctx) const;

    const Config config_;
    std::shared_ptr<basic::Scheduler> scheduler_;
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

    /// Renew addresses in address repo periodically within heartbeat timer
    std::chrono::milliseconds addresses_renewal_time_{0};

    /// Logger
    log::SubLogger log_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_CONNECTIVITY_HPP
