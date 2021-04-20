/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_DIALER_IMPL_HPP
#define LIBP2P_DIALER_IMPL_HPP

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/dialer.hpp>
#include <libp2p/network/listener_manager.hpp>
#include <libp2p/network/transport_manager.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::network {

  class DialerImpl : public Dialer {
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
    void newStream(const peer::PeerInfo &p, const peer::Protocol &protocol,
                   StreamResultFunc cb,
                   std::chrono::milliseconds timeout) override;

    void newStream(const peer::PeerId &peer_id, const peer::Protocol &protocol,
                   StreamResultFunc cb) override;

   private:
    std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect_;
    std::shared_ptr<TransportManager> tmgr_;
    std::shared_ptr<ConnectionManager> cmgr_;
    std::shared_ptr<ListenerManager> listener_;
    std::shared_ptr<basic::Scheduler> scheduler_;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_DIALER_IMPL_HPP
