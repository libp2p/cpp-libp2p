/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTISELECT_IMPL_HPP
#define LIBP2P_MULTISELECT_IMPL_HPP

#include <memory>
#include <queue>
#include <string_view>
#include <vector>

#include <boost/core/noncopyable.hpp>
#include <gsl/span>
#include <libp2p/common/logger.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/protocol_muxer/multiselect/message_manager.hpp>
#include <libp2p/protocol_muxer/multiselect/message_reader.hpp>
#include <libp2p/protocol_muxer/multiselect/message_writer.hpp>
#include <libp2p/protocol_muxer/multiselect/multiselect_error.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::protocol_muxer {
  /**
   * Implementation of a protocol muxer. Read more
   * https://github.com/multiformats/multistream-select
   */
  class Multiselect : public ProtocolMuxer,
                      public std::enable_shared_from_this<Multiselect>,
                      private boost::noncopyable {
    friend MessageWriter;
    friend MessageReader;

   public:
    ~Multiselect() override = default;

    void selectOneOf(gsl::span<const peer::Protocol> protocols,
                     std::shared_ptr<basic::ReadWriter> connection,
                     bool is_initiator, ProtocolHandlerFunc cb) override;

   private:
    /**
     * Negotiate about a protocol
     * @param connection to be negotiated over
     * @param round, about which protocol the negotiation is to take place
     * @return chosen protocol in case of success, error otherwise
     */
    void negotiate(const std::shared_ptr<basic::ReadWriter> &connection,
                   gsl::span<const peer::Protocol> protocols, bool is_initiator,
                   const ProtocolHandlerFunc &handler);

    /**
     * Triggered, when error happens during the negotiation round
     * @param connection_state - state of the connection
     * @param ec - error, which happened
     */
    void negotiationRoundFailed(
        const std::shared_ptr<ConnectionState> &connection_state,
        const std::error_code &ec);

    void onWriteCompleted(
        std::shared_ptr<ConnectionState> connection_state) const;

    void onWriteAckCompleted(
        const std::shared_ptr<ConnectionState> &connection_state,
        const peer::Protocol &protocol);

    void onReadCompleted(std::shared_ptr<ConnectionState> connection_state,
                         MessageManager::MultiselectMessage msg);

    void handleOpeningMsg(std::shared_ptr<ConnectionState> connection_state);

    void handleProtocolMsg(
        const peer::Protocol &protocol,
        const std::shared_ptr<ConnectionState> &connection_state);

    void handleProtocolsMsg(
        const std::vector<peer::Protocol> &protocols,
        const std::shared_ptr<ConnectionState> &connection_state);

    void onProtocolAfterOpeningLsOrNa(
        std::shared_ptr<ConnectionState> connection_state,
        const peer::Protocol &protocol);

    void onProtocolsAfterLs(
        const std::shared_ptr<ConnectionState> &connection_state,
        gsl::span<const peer::Protocol> received_protocols);

    void handleLsMsg(const std::shared_ptr<ConnectionState> &connection_state);

    void handleNaMsg(
        const std::shared_ptr<ConnectionState> &connection_state) const;

    void onUnexpectedRequestResponse(
        const std::shared_ptr<ConnectionState> &connection_state);

    void onGarbagedStreamStatus(
        const std::shared_ptr<ConnectionState> &connection_state);

    void negotiationRoundFinished(
        const std::shared_ptr<ConnectionState> &connection_state,
        const peer::Protocol &chosen_protocol);

    std::tuple<std::shared_ptr<common::ByteArray>,
               std::shared_ptr<boost::asio::streambuf>, size_t>
    getBuffers();

    void clearResources(
        const std::shared_ptr<ConnectionState> &connection_state);

    std::vector<std::shared_ptr<common::ByteArray>> write_buffers_;
    std::vector<std::shared_ptr<boost::asio::streambuf>> read_buffers_;
    std::queue<size_t> free_buffers_;

    // TODO(warchant): use logger interface here and inject it PRE-235
    libp2p::common::Logger log_ = libp2p::common::createLogger("multiselect");
  };
}  // namespace libp2p::protocol_muxer

#endif  // LIBP2P_MULTISELECT_IMPL_HPP
