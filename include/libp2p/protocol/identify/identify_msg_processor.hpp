/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_IDENTIFY_MSG_PROCESSOR_HPP
#define LIBP2P_IDENTIFY_MSG_PROCESSOR_HPP

#include <memory>
#include <optional>
#include <string>

#include <gsl/span>
#include <libp2p/connection/stream.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/identify/observed_addresses.hpp>

namespace identify::pb {
  class Identify;
}

namespace libp2p::protocol {
  /**
   * Processor of messages of Identify protocol
   */
  class IdentifyMessageProcessor
      : public std::enable_shared_from_this<IdentifyMessageProcessor> {
    using StreamSPtr = std::shared_ptr<connection::Stream>;

   public:
    using IdentifyCallback = void(const peer::PeerId &);

    IdentifyMessageProcessor(
        Host &host, network::ConnectionManager &conn_manager,
        peer::IdentityManager &identity_manager,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    boost::signals2::connection onIdentifyReceived(
        const std::function<IdentifyCallback> &cb);

    /**
     * Send an Identify message over the provided stream
     * @param stream to be identified over
     */
    void sendIdentify(StreamSPtr stream);

    /**
     * Receive an Identify message from the provided stream
     * @param stream to be identified over
     */
    void receiveIdentify(StreamSPtr stream);

    /**
     * Get a Host of this processor
     * @return Host
     */
    Host &getHost() const noexcept;

    /**
     * Get a ConnectionManager of this processor
     * @return ConnectionManager
     */
    network::ConnectionManager &getConnectionManager() const noexcept;

    /**
     * Get an ObservedAddresses of this processor
     * @return ObservedAddresses
     */
    const ObservedAddresses &getObservedAddresses() const noexcept;

   private:
    /**
     * Called, when an identify message is written to the stream
     * @param written_bytes - how much bytes were written
     * @param stream with the other side
     */
    void identifySent(outcome::result<size_t> written_bytes,
                      const StreamSPtr &stream);

    /**
     * Called, when an identify message is received from the other peer
     * @param msg, which was read
     * @param stream, over which it was received
     */
    void identifyReceived(outcome::result<identify::pb::Identify> msg,
                          const StreamSPtr &stream);

    /**
     * Process a received public key of the other peer
     * @param stream, over which the key was received
     * @param pubkey_str - marshalled public key; can be empty, if there was no
     * public key in the message
     * @return peer id, which was derived from the provided public key (if it
     * can be derived)
     */
    boost::optional<peer::PeerId> consumePublicKey(const StreamSPtr &stream,
                                                   std::string_view pubkey_str);

    /**
     * Process received address, which the other peer used to connect to us
     * @param address - observed address string
     * @param peer_id - ID of that peer
     * @param stream, over which the message came
     */
    void consumeObservedAddresses(const std::string &address_str,
                                  const peer::PeerId &peer_id,
                                  const StreamSPtr &stream);

    /**
     * Check if provided multiaddress has the same set of transports as at least
     * of the (\param mas)
     * @param ma - address to be checked
     * @param mas - addresses to be checked against
     * @return true, if that address has common transports, false otherwise
     */
    bool hasConsistentTransport(const multi::Multiaddress &ma,
                                gsl::span<const multi::Multiaddress> mas);

    /**
     * Process received addresses, which the other peer listens to
     * @param addresses_strings - stringified listen addresses
     * @param peer_id - ID of that peer
     */
    void consumeListenAddresses(gsl::span<const std::string> addresses_strings,
                                const peer::PeerId &peer_id);

    Host &host_;
    network::ConnectionManager &conn_manager_;
    peer::IdentityManager &identity_manager_;
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;
    ObservedAddresses observed_addresses_;
    boost::signals2::signal<IdentifyCallback> signal_identify_received_;

    log::Logger log_ = log::createLogger("IdentifyMsgProcessor");
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_IDENTIFY_MSG_PROCESSOR_HPP
