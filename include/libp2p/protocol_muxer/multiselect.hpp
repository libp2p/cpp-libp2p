/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_MUXER_MULTISELECT_HPP
#define LIBP2P_PROTOCOL_MUXER_MULTISELECT_HPP

#include "protocol_muxer.hpp"

#include "multiselect/parser.hpp"

namespace libp2p::protocol_muxer::multiselect {

  class Multiselect : public protocol_muxer::ProtocolMuxer,
                      public std::enable_shared_from_this<Multiselect> {
   public:
    ~Multiselect() override = default;

    /// Implements ProtocolMuxer API
    void selectOneOf(gsl::span<const peer::Protocol> protocols,
                     std::shared_ptr<basic::ReadWriter> connection,
                     bool is_initiator, bool negotiate_multiselect,
                     ProtocolHandlerFunc cb) override;

   private:
    using Protocols = boost::container::small_vector<std::string, 4>;
    using Packet = std::shared_ptr<MsgBuf>;
    using Parser = detail::Parser;

    void sendProposal(bool defer_cb = false);

    void send(MsgBuf msg);

    void onDataWritten(outcome::result<size_t> res);

    void close(outcome::result<std::string> result);

    void receive();

    void onDataRead(outcome::result<size_t> res);

    boost::optional<outcome::result<std::string>> onMessagesRead();

    size_t current_round_ = 0;

    Protocols protocols_;
    std::shared_ptr<basic::ReadWriter> connection_;
    ProtocolHandlerFunc callback_;

    bool is_initiator_ = false;
    bool multistream_negotiated_ = false;
    bool proposal_was_sent_ = false;
    bool closed_ = false;

    size_t current_protocol_ = 0;

    boost::optional<size_t> protocol_reply_was_sent_;

    Parser parser_;

    boost::optional<Packet> ls_response_;
    boost::optional<Packet> na_response_;

    std::shared_ptr<std::array<uint8_t, kMaxMessageSize>> read_buffer_;

    // underlying ReadWriter may not support buffered writes
    std::deque<MsgBuf> write_queue_;
  };

}  // namespace libp2p::protocol_muxer::multiselect

#endif  // LIBP2P_PROTOCOL_MUXER_MULTISELECT_HPP
