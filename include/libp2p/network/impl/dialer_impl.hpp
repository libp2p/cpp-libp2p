/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_DIALER_IMPL_HPP
#define LIBP2P_DIALER_IMPL_HPP

#include <set>
#include <unordered_map>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/dialer.hpp>
#include <libp2p/network/listener_manager.hpp>
#include <libp2p/network/transport_manager.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::network {

  class DialerImpl : public Dialer,
                     public std::enable_shared_from_this<DialerImpl> {
   public:
    ~DialerImpl() override = default;

    DialerImpl(std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect,
               std::shared_ptr<TransportManager> tmgr,
               std::shared_ptr<ConnectionManager> cmgr,
               std::shared_ptr<ListenerManager> listener,
               std::shared_ptr<basic::Scheduler> scheduler);

    // Establishes a connection to a given peer
    void dial(const peer::PeerInfo &p, DialResultFunc cb,
              std::chrono::milliseconds timeout) override;

    // NewStream returns a new stream to given peer p.
    // If there is no connection to p, attempts to create one.
    void newStream(const peer::PeerInfo &p, StreamProtocols protocols,
                   StreamAndProtocolOrErrorCb cb,
                   std::chrono::milliseconds timeout = {}) override;

    void newStream(const peer::PeerId &peer_id, StreamProtocols protocols,
                   StreamAndProtocolOrErrorCb cb) override;

   private:
    // A context to handle an intermediary state of the peer we are dialing to
    // but the connection is not yet established
    struct DialCtx {
      /// Known and scheduled addresses to try to dial via
      std::set<multi::Multiaddress> addresses;

      /// Timeout for a single connection attempt
      std::chrono::milliseconds timeout;

      /// Addresses we already tried, but no connection was established
      std::set<multi::Multiaddress> tried_addresses;

      /// Callbacks for all who requested a connection to the peer
      std::vector<Dialer::DialResultFunc> callbacks;

      /// Result temporary storage to propagate via callbacks
      boost::optional<DialResult> result;
      // ^ used when all connecting attempts failed and no more known peer
      // addresses are left

      // indicates that at least one attempt to dial was happened
      // (at least one supported network transport was found and used)
      bool dialled = false;
    };

    // Perform a single attempt to dial to the peer via the next known address
    void rotate(const peer::PeerId &peer_id);

    // Finalize dialing to the peer and propagate a given result to all
    // connection requesters
    void completeDial(const peer::PeerId &peer_id, const DialResult &result);

    void newStream(std::shared_ptr<connection::CapableConnection> conn,
                   StreamProtocols protocols, StreamAndProtocolOrErrorCb cb);

    std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect_;
    std::shared_ptr<TransportManager> tmgr_;
    std::shared_ptr<ConnectionManager> cmgr_;
    std::shared_ptr<ListenerManager> listener_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    log::Logger log_;

    // peers we are currently dialing to
    std::unordered_map<peer::PeerId, DialCtx> dialing_peers_;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_DIALER_IMPL_HPP
