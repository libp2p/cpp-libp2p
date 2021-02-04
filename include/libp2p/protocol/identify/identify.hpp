/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_IDENTIFY_IMPL_HPP
#define LIBP2P_IDENTIFY_IMPL_HPP

#include <memory>
#include <vector>

#include <libp2p/event/bus.hpp>
#include <libp2p/protocol/base_protocol.hpp>
#include <libp2p/protocol/identify/identify_msg_processor.hpp>

namespace libp2p::multi {
  class Multiaddress;
}

namespace libp2p::protocol {
  /**
   * Implementation of an Identify protocol, which is a way to say
   * "hello" to the other peer, sending our listen addresses, ID, etc
   * Read more: https://github.com/libp2p/specs/tree/master/identify
   */
  class Identify : public BaseProtocol,
                   public std::enable_shared_from_this<Identify> {
   public:
    /**
     * Create an Identify instance; it will immediately start watching
     * connection events and react to them
     * @param msg_processor to work with Identify messages
     * @param event_bus - bus, over which the events arrive
     */
    Identify(Host &host,
             std::shared_ptr<IdentifyMessageProcessor> msg_processor,
             event::Bus &event_bus);

    ~Identify() override = default;

    boost::signals2::connection onIdentifyReceived(
        const std::function<IdentifyMessageProcessor::IdentifyCallback> &cb);

    /**
     * Get addresses other peers reported we have dialed from
     * @return set of addresses
     */
    std::vector<multi::Multiaddress> getAllObservedAddresses() const;

    /**
     * Get addresses other peers reported we have dialed from, when they
     * provided a (\param address)
     * @param address, for which to retrieve observed addresses
     * @return set of addresses
     */
    std::vector<multi::Multiaddress> getObservedAddressesFor(
        const multi::Multiaddress &address) const;

    peer::Protocol getProtocolId() const override;

    /**
     * In Identify, handle means we are being identified by the other peer, so
     * we are expected to send the Identify message
     */
    void handle(StreamResult stream_res) override;

    /**
     * Start accepting NewConnectionEvent-s and asking each of them for Identify
     */
    void start();

   private:
    /**
     * Handler for new connections, established by or with our host
     * @param conn - new connection
     */
    void onNewConnection(
        const std::weak_ptr<connection::CapableConnection> &conn);

    Host &host_;
    std::shared_ptr<IdentifyMessageProcessor> msg_processor_;
    event::Bus &bus_;
    event::Handle sub_;  // will unsubscribe during destruction by itself

    bool started_ = false;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_IDENTIFY_IMPL_HPP
