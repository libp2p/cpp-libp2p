/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/multiselect_instance.hpp>

#include <cctype>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/trace.hpp>
#include <libp2p/protocol_muxer/multiselect/serializing.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::protocol_muxer::multiselect {

  namespace {
    const log::Logger &log() {
      static log::Logger logger = log::createLogger("Multiselect");
      return logger;
    }
  }  // namespace

  MultiselectInstance::MultiselectInstance(Multiselect &owner)
      : owner_(owner) {}

  void MultiselectInstance::selectOneOf(
      gsl::span<const peer::ProtocolName> protocols,
      std::shared_ptr<basic::ReadWriter> connection, bool is_initiator,
      bool negotiate_multiselect, Multiselect::ProtocolHandlerFunc cb) {
    assert(!protocols.empty());
    assert(connection);
    assert(cb);

    protocols_.assign(protocols.begin(), protocols.end());

    connection_ = std::move(connection);

    callback_ = std::move(cb);

    is_initiator_ = is_initiator;

    multistream_negotiated_ = !negotiate_multiselect;

    wait_for_protocol_reply_ = false;

    closed_ = false;

    current_protocol_ = 0;

    wait_for_reply_sent_.reset();

    parser_.reset();

    if (!read_buffer_) {
      read_buffer_ = std::make_shared<std::array<uint8_t, kMaxMessageSize>>();
    }

    write_queue_.clear();
    is_writing_ = false;

    if (is_initiator_) {
      std::ignore = sendProposal();
    } else if (negotiate_multiselect) {
      sendOpening();
    }

    receive();
  }

  void MultiselectInstance::sendOpening() {
    if (is_initiator_) {
      sendProposal();
    } else {
      send(detail::createMessage(kProtocolId));
    }
  }

  bool MultiselectInstance::sendProposal() {
    if (current_protocol_ >= protocols_.size()) {
      // nothing more to propose
      SL_DEBUG(log(), "none of proposed protocols were accepted by peer");
      return false;
    }

    if (!multistream_negotiated_) {
      std::array<std::string_view, 2> a(
          {kProtocolId, protocols_[current_protocol_]});
      send(detail::createMessage(a, false));
    } else {
      send(detail::createMessage(protocols_[current_protocol_]));
    }

    wait_for_protocol_reply_ = true;
    return true;
  }

  void MultiselectInstance::sendNA() {
    if (!na_response_) {
      na_response_ =
          std::make_shared<MsgBuf>(detail::createMessage(kNA).value());
    }
    send(na_response_.value());
  }

  void MultiselectInstance::send(outcome::result<MsgBuf> msg) {
    if (!msg) {
      return connection_->deferWriteCallback(
          msg.error(),
          [wptr = weak_from_this(),
           round = current_round_](outcome::result<size_t> res) {
            auto self = wptr.lock();
            if (self && self->current_round_ == round) {
              self->onDataWritten(res);
            }
          });
    }
    send(std::make_shared<MsgBuf>(std::move(msg.value())));
  }

  void MultiselectInstance::send(Packet packet) {
    if (is_writing_) {
      write_queue_.push_back(std::move(packet));
      return;
    }

    auto span = gsl::span<const uint8_t>(*packet);

    SL_TRACE(log(), "sending {}", common::dumpBin(span));

    connection_->write(
        span, span.size(),
        [wptr = weak_from_this(), round = current_round_,
         packet = std::move(packet)](outcome::result<size_t> res) {
          auto self = wptr.lock();
          if (self && self->current_round_ == round) {
            self->onDataWritten(res);
          }
        });

    is_writing_ = true;
  }

  void MultiselectInstance::onDataWritten(outcome::result<size_t> res) {
    is_writing_ = false;

    if (!res) {
      return close(res.error());
    }
    if (!closed_) {
      if (!write_queue_.empty()) {
        send(std::move(write_queue_.front()));
        write_queue_.pop_front();
        return;
      }

      if (wait_for_reply_sent_.has_value()) {
        // reply was sent successfully, closing with success
        return close(protocols_[wait_for_reply_sent_.value()]);
      }
    }
  }

  void MultiselectInstance::close(outcome::result<std::string> result) {
    closed_ = true;
    ++current_round_;
    write_queue_.clear();
    Multiselect::ProtocolHandlerFunc callback;
    callback.swap(callback_);

    owner_.instanceClosed(shared_from_this(), callback, std::move(result));
  }

  void MultiselectInstance::receive() {
    if (closed_ || parser_.state() != Parser::kUnderflow) {
      log()->error("receive(): invalid state");
      return;
    }

    auto bytes_needed = parser_.bytesNeeded();

    assert(bytes_needed > 0);

    if (bytes_needed > kMaxMessageSize) {
      SL_TRACE(log(), "rejecting incoming traffic, too large message ({})",
               bytes_needed);
      return close(ProtocolMuxer::Error::PROTOCOL_VIOLATION);
    }

    gsl::span<uint8_t> span(*read_buffer_);
    span = span.first(static_cast<Parser::IndexType>(bytes_needed));

    connection_->read(span, bytes_needed,
                      [wptr = weak_from_this(), round = current_round_,
                       packet = read_buffer_](outcome::result<size_t> res) {
                        auto self = wptr.lock();
                        if (self && self->current_round_ == round) {
                          self->onDataRead(res);
                        }
                      });
  }

  void MultiselectInstance::onDataRead(outcome::result<size_t> res) {
    if (!res) {
      return close(res.error());
    }

    auto bytes_read = res.value();
    if (bytes_read > read_buffer_->size()) {
      log()->error("onDataRead(): invalid state");
      return close(ProtocolMuxer::Error::INTERNAL_ERROR);
    }

    gsl::span<const uint8_t> span(*read_buffer_);
    span = span.first(static_cast<Parser::IndexType>(bytes_read));

    SL_TRACE(log(), "received {}", common::dumpBin(span));

    boost::optional<outcome::result<std::string>> got_result;

    auto state = parser_.consume(span);
    switch (state) {
      case Parser::kUnderflow:
        break;
      case Parser::kReady:
        got_result = processMessages();
        break;
      default:
        SL_TRACE(log(), "peer error: parser overflow");
        got_result = ProtocolMuxer::Error::PROTOCOL_VIOLATION;
        break;
    }

    if (got_result) {
      return close(got_result.value());
    }

    if (!wait_for_reply_sent_) {
      receive();
    }
  }

  MultiselectInstance::MaybeResult MultiselectInstance::processMessages() {
    MaybeResult result;

    for (const auto &msg : parser_.messages()) {
      switch (msg.type) {
        case Message::kProtocolName:
          result = handleProposal(msg.content);
          break;
        case Message::kRightProtocolVersion:
          multistream_negotiated_ = true;
          break;
        case Message::kNAMessage:
          result = handleNA();
          break;
        case Message::kWrongProtocolVersion: {
          SL_DEBUG(log(), "Received unsupported protocol version: {}",
                   common::dumpBin(msg.content));
          result = ProtocolMuxer::Error::PROTOCOL_VIOLATION;
        } break;
        default: {
          SL_DEBUG(log(), "Received invalid message: {}",
                   common::dumpBin(msg.content));
          result = ProtocolMuxer::Error::PROTOCOL_VIOLATION;
        } break;
      }

      if (result) {
        break;
      }
    }

    parser_.reset();
    return result;
  }

  MultiselectInstance::MaybeResult MultiselectInstance::handleProposal(
      const std::string_view &protocol) {
    if (is_initiator_) {
      if (wait_for_protocol_reply_) {
        assert(current_protocol_ < protocols_.size());

        if (protocols_[current_protocol_] == protocol) {
          // successful client side negotiation
          return MaybeResult(std::string(protocol));
        }
      }

      SL_DEBUG(log(), "Unexpected message received by client: {}",
               common::dumpBin(protocol));
      return MaybeResult(ProtocolMuxer::Error::PROTOCOL_VIOLATION);
    }

    // server side

    size_t idx = 0;
    for (const auto &p : protocols_) {
      if (p == protocol) {
        // successful server side negotiation
        wait_for_reply_sent_ = idx;
        write_queue_.clear();
        send(detail::createMessage(protocol));
        break;
      }
      ++idx;
    }

    if (!wait_for_reply_sent_) {
      SL_DEBUG(log(), "unknown protocol {} proposed by client", protocol);
      sendNA();
    }

    return boost::none;
  }

  MultiselectInstance::MaybeResult MultiselectInstance::handleNA() {
    if (is_initiator_) {
      if (current_protocol_ < protocols_.size()) {
        SL_DEBUG(log(), "protocol {} was not accepted by peer",
                 protocols_[current_protocol_]);
      }

      ++current_protocol_;

      if (sendProposal()) {
        // will try the next protocol
        return boost::none;
      }

      SL_DEBUG(log(), "Failed to negotiate protocols: {}",
               fmt::join(protocols_.begin(), protocols_.end(), ", "));
      return MaybeResult(ProtocolMuxer::Error::NEGOTIATION_FAILED);
    }

    // server side

    SL_DEBUG(log(), "Unexpected NA received by server");
    return MaybeResult(ProtocolMuxer::Error::PROTOCOL_VIOLATION);
  }

}  // namespace libp2p::protocol_muxer::multiselect
