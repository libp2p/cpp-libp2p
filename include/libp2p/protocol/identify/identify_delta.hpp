/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_IDENTIFY_DELTA_HPP
#define LIBP2P_IDENTIFY_DELTA_HPP

#include <vector>

#include <libp2p/event/bus.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/router.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/protocol_repository.hpp>
#include <libp2p/protocol/base_protocol.hpp>

namespace identify::pb {
  class Identify;
}

namespace libp2p::protocol {
  /**
   * Identify-Delta is used to notify other peers about changes in our supported
   * protocols. Read more:
   * https://github.com/libp2p/specs/blob/master/identify/README.md
   */
  class IdentifyDelta : public BaseProtocol,
                        public std::enable_shared_from_this<IdentifyDelta> {
   public:
    /**
     * Create an instance of Identify-Delta
     * @param host - this Libp2p node
     * @param conn_manager - connection manager of this node
     * @param bus, over which the events arrive
     */
    IdentifyDelta(Host &host, network::ConnectionManager &conn_manager,
                  event::Bus &bus);

    peer::Protocol getProtocolId() const override;

    /**
     * In Identify-Delta, handle means we accepted an Identify-Delta stream and
     * should receive a Delta message
     */
    void handle(StreamResult stream_res) override;

    /**
     * Start this Identify-Delta, so that it subscribes to events, when
     * list of protocols our peer supports changes
     */
    void start();

   private:
    /**
     * Process a received Delta message
     * @param msg_res - result to the received message
     * @param stream, over which the message or error was received
     */
    void deltaReceived(outcome::result<identify::pb::Identify> msg_res,
                       const std::shared_ptr<connection::Stream> &stream);

    /**
     * Send a Delta message
     * @param added protocols
     * @param removed protocols
     */
    void sendDelta(gsl::span<const peer::Protocol> added,
                   gsl::span<const peer::Protocol> removed);

    /**
     * Send a Delta message
     * @param stream for the message to be sent over
     * @param msg to be sent
     */
    void sendDelta(std::shared_ptr<connection::Stream> stream,
                   const std::shared_ptr<identify::pb::Identify> &msg) const;

    Host &host_;
    network::ConnectionManager &conn_manager_;
    event::Bus &bus_;

    event::Handle new_protos_sub_;
    event::Handle rm_protos_sub_;

    libp2p::log::Logger log_ = libp2p::log::createLogger("IdentifyDelta");
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_IDENTIFY_DELTA_HPP
