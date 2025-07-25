/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/multiselect_instance.hpp>

#include <cctype>
#include <span>

#include <libp2p/basic/read.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/common/trace.hpp>
#include <libp2p/protocol_muxer/multiselect/serializing.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::protocol_muxer::multiselect {

  namespace {
    const log::Logger &log() {
      static log::Logger logger = log::createLogger("Multiselect");
      return logger;
    }

    /// Timeout for protocol negotiation (5 seconds)
    constexpr std::chrono::milliseconds kNegotiationTimeout{5000};
  }  // namespace

  MultiselectInstance::MultiselectInstance(
      Multiselect &owner, std::shared_ptr<basic::Scheduler> scheduler)
      : owner_(owner), scheduler_(std::move(scheduler)) {}

  void MultiselectInstance::selectOneOf(
      std::span<const peer::ProtocolName> protocols,
      std::shared_ptr<basic::ReadWriter> connection,
      bool is_initiator,
      bool negotiate_multiselect,
      Multiselect::ProtocolHandlerFunc cb) {
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

    // Schedule timeout for negotiation
    timeout_handle_ = scheduler_->scheduleWithHandle(
        [wptr = std::weak_ptr<MultiselectInstance>(shared_from_this())]() {
          if (auto self = wptr.lock()) {
            self->onTimeout();
          }
        },
        kNegotiationTimeout);

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
          std::error_code{},
          [wptr = weak_from_this(),
           res = std::move(msg.error()),
           round = current_round_](outcome::result<size_t>) mutable {
            auto self = wptr.lock();
            if (self && self->current_round_ == round) {
              self->onDataWritten(std::move(res));
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

    write(connection_,
          *packet,
          [wptr = weak_from_this(), round = current_round_, packet](
              outcome::result<void> res) {
            auto self = wptr.lock();
            if (self && self->current_round_ == round) {
              self->onDataWritten(res);
            }
          });

    is_writing_ = true;
  }

  void MultiselectInstance::onDataWritten(outcome::result<void> res) {
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

    // Cancel timeout if it's still active
    timeout_handle_.reset();

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
      SL_TRACE(log(),
               "rejecting incoming traffic, too large message ({})",
               bytes_needed);
      return close(ProtocolMuxer::Error::PROTOCOL_VIOLATION);
    }

    BytesOut span(*read_buffer_);
    span = span.first(static_cast<Parser::IndexType>(bytes_needed));

    libp2p::read(connection_,
                 span,
                 [wptr = weak_from_this(),
                  round = current_round_,
                  packet = read_buffer_](outcome::result<void> res) {
                   auto self = wptr.lock();
                   if (self && self->current_round_ == round) {
                     self->onDataRead(res);
                   }
                 });
  }

  void MultiselectInstance::onDataRead(outcome::result<void> res) {
    if (!res) {
      return close(res.error());
    }

    BytesIn span(*read_buffer_);
    span = span.first(static_cast<Parser::IndexType>(parser_.bytesNeeded()));

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
          result = ProtocolMuxer::Error::PROTOCOL_VIOLATION;
        } break;
        default: {
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
        SL_DEBUG(log(),
                 "protocol {} was not accepted by peer",
                 protocols_[current_protocol_]);
      }

      ++current_protocol_;

      if (sendProposal()) {
        // will try the next protocol
        return boost::none;
      }

      SL_DEBUG(log(),
               "Failed to negotiate protocols: {}",
               fmt::join(protocols_, ", "));
      return MaybeResult(ProtocolMuxer::Error::NEGOTIATION_FAILED);
    }

    // server side

    SL_DEBUG(log(), "Unexpected NA received by server");
    return MaybeResult(ProtocolMuxer::Error::PROTOCOL_VIOLATION);
  }

  void MultiselectInstance::onTimeout() {
    SL_DEBUG(log(),
             "Protocol negotiation timeout after {}ms",
             kNegotiationTimeout.count());
    close(ProtocolMuxer::Error::NEGOTIATION_FAILED);
  }

}  // namespace libp2p::protocol_muxer::multiselect
