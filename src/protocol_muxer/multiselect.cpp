/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect.hpp>

#include <libp2p/protocol_muxer/multiselect/serializing.hpp>

namespace libp2p::protocol_muxer::multiselect {

  void Multiselect::selectOneOf(gsl::span<const peer::Protocol> protocols,
                                std::shared_ptr<basic::ReadWriter> connection,
                                bool is_initiator, bool negotiate_multiselect,
                                ProtocolHandlerFunc cb) {
    assert(!protocols.empty());
    assert(connection);
    assert(cb);

    ++current_round_;

    if (protocols.empty()) {
      // ~~~log and defer

      return;
    }

    protocols_.assign(protocols.begin(), protocols.end());
    connection_ = std::move(connection);
    is_initiator_ = is_initiator;
    callback_ = std::move(cb);

    multistream_negotiated_ = false;
    proposal_was_sent_ = false;
    closed_ = false;
    current_protocol_ = 0;
    protocol_reply_was_sent_.reset();
    parser_.reset();
    ls_response_.reset();
    read_buffer_ = std::make_shared<std::array<uint8_t, kMaxMessageSize>>();
    write_queue_.clear();

    if (is_initiator_) {
      sendProposal(true);
    }

    receive();
  }

  void Multiselect::sendProposal(bool defer_cb) {
    if (current_protocol_ >= protocols_.size()) {
      // nothing to propose
      // TODO defer cb
      return;
    }
    MsgBuf msg;
    if (!multistream_negotiated_) {
      std::array<std::string_view, 2> a(
          {kProtocolId, protocols_[current_protocol_]});
      msg = detail::createMessage(a, true);  // TODO nested or not ???
    } else {
      msg = detail::createMessage(protocols_[current_protocol_]);
    }

    ++current_protocol_;
    proposal_was_sent_ = true;

    send(std::move(msg));
  }

  void Multiselect::send(MsgBuf msg) {
    auto packet = std::make_shared<MsgBuf>(std::move(msg));
    auto span = gsl::span<const uint8_t>(*packet);
    connection_->write(
        span, span.size(),
        [wptr = weak_from_this(), round = current_round_,
         packet = std::move(packet)](outcome::result<size_t> res) {
          auto self = wptr.lock();
          // TODO logs and traces
          if (self && self->current_round_ == round) {
            self->onDataWritten(res);
          }
        });
  }

  void Multiselect::onDataWritten(outcome::result<size_t> res) {
    if (!res) {
      return close(res.error());
    }
    if (!closed_) {
      if (!write_queue_.empty()) {
        send(std::move(write_queue_.front()));
        write_queue_.pop_front();
        return;
      }

      if (protocol_reply_was_sent_.has_value()) {
        // reply was sent successfully, closing with success
        return close(protocols_[protocol_reply_was_sent_.value()]);
      }
    }
  }

  void Multiselect::close(outcome::result<std::string> result) {
    closed_ = true;
    ProtocolHandlerFunc callback;
    callback.swap(callback_);
    callback(std::move(result));
  }

  void Multiselect::receive() {
    if (closed_ || parser_.state() != Parser::kUnderflow) {
      // TODO warning if not underflow, it was not reset
      return;
    }

    size_t bytes_needed = parser_.bytesNeeded();

    assert(bytes_needed > 0);

    if (bytes_needed > kMaxMessageSize) {
      // ~~~log
      return close(Error::PROTOCOL_VIOLATION);
    }

    gsl::span<uint8_t> span(*read_buffer_);
    span = span.first(bytes_needed);

    connection_->read(span, bytes_needed,
                      [wptr = weak_from_this(), round = current_round_,
                       packet = read_buffer_](outcome::result<size_t> res) {
                        auto self = wptr.lock();
                        // TODO logs and traces
                        if (self && self->current_round_ == round) {
                          self->onDataRead(res);
                        }
                      });
  }

  void Multiselect::onDataRead(outcome::result<size_t> res) {
    if (!res) {
      close(res.error());
    }

    size_t bytes_read = res.value();
    if (bytes_read > read_buffer_->size()) {
      // ~~~log
      return close(Error::INTERNAL_ERROR);
    }

    gsl::span<const uint8_t> span(*read_buffer_);
    span = span.first(bytes_read);

    boost::optional<outcome::result<std::string>> got_result;

    auto state = parser_.consume(span);
    switch (state) {
      case Parser::kUnderflow:
        break;
      case Parser::kReady:
        got_result = onMessagesRead();
        break;
      default:
        // ~~~log
        got_result = Error::PROTOCOL_VIOLATION;
        break;
    }

    if (got_result) {
      return close(got_result.value());
    }

    receive();
  }

  boost::optional<outcome::result<std::string>> Multiselect::onMessagesRead() {
    /*

     if (!multistream_negotiated_) expect right protocol in 1st message

     if (ls) send ls

     if (na) propose further

     if (protocol) than it depends of is_initiator

     */

    return boost::none;
  }

}  // namespace libp2p::protocol_muxer::multiselect
