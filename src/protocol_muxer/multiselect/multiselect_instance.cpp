/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/multiselect_instance.hpp>

#include <cctype>

#include <boost/asio/use_awaitable.hpp>
#include <libp2p/basic/write_return_size.hpp>
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

    auto span = BytesIn(*packet);

    writeReturnSize(connection_,
                    span,
                    [wptr = weak_from_this(),
                     round = current_round_,
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
      SL_TRACE(log(),
               "rejecting incoming traffic, too large message ({})",
               bytes_needed);
      return close(ProtocolMuxer::Error::PROTOCOL_VIOLATION);
    }

    BytesOut span(*read_buffer_);
    span = span.first(static_cast<Parser::IndexType>(bytes_needed));

    connection_->read(span,
                      bytes_needed,
                      [wptr = weak_from_this(),
                       round = current_round_,
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

    BytesIn span(*read_buffer_);
    span = span.first(static_cast<Parser::IndexType>(bytes_read));

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

  boost::asio::awaitable<outcome::result<peer::ProtocolName>>
  MultiselectInstance::selectOneOf(
      std::span<const peer::ProtocolName> protocols,
      std::shared_ptr<basic::ReadWriter> connection,
      bool is_initiator,
      bool negotiate_multiselect) {
    assert(!protocols.empty());
    assert(connection);

    // Use local variables instead of class members for the coroutine
    // implementation
    bool multistream_negotiated = !negotiate_multiselect;
    bool wait_for_protocol_reply = false;
    size_t current_protocol = 0;
    boost::optional<size_t> wait_for_reply_sent;

    // Store protocols in a local variable instead of using the member variable
    boost::container::small_vector<std::string, 4> local_protocols(
        protocols.begin(), protocols.end());

    // Only reset the parser since it's used for state tracking
    parser_.reset();

    // Local read buffer - avoid using the shared class member
    auto read_buffer = std::make_shared<std::array<uint8_t, kMaxMessageSize>>();

    // Initial protocol negotiation
    if (is_initiator) {
      // Send the first protocol proposal
      auto result =
          co_await sendProtocolProposalCoro(connection,
                                            multistream_negotiated,
                                            local_protocols[current_protocol]);
      if (!result) {
        co_return result.error();
      }
      wait_for_protocol_reply = true;
    } else if (negotiate_multiselect) {
      // Send opening protocol ID (server side)
      auto msg = detail::createMessage(kProtocolId);
      if (!msg) {
        co_return msg.error();
      }
      auto packet = std::make_shared<MsgBuf>(msg.value());
      try {
        auto ec =
            co_await connection->writeSome(BytesIn(*packet), packet->size());
        if (ec) {
          co_return ec;
        }
      } catch (const std::exception &e) {
        log()->error("Error writing opening protocol ID: {}", e.what());
        co_return std::make_error_code(std::errc::io_error);
      }
    }

    // Cache for NA response - local to this coroutine
    boost::optional<std::shared_ptr<MsgBuf>> na_response;

    // Main negotiation loop
    while (true) {
      // Read data from the connection
      auto bytes_needed = parser_.bytesNeeded();
      if (bytes_needed > kMaxMessageSize) {
        SL_TRACE(log(),
                 "rejecting incoming traffic, too large message ({})",
                 bytes_needed);
        co_return ProtocolMuxer::Error::PROTOCOL_VIOLATION;
      }

      BytesOut span(*read_buffer);
      span = span.first(static_cast<Parser::IndexType>(bytes_needed));

      try {
        auto read_result = co_await connection->read(span, bytes_needed);
        if (!read_result) {
          co_return read_result.error();
        }

        auto bytes_read = read_result.value();
        if (bytes_read > read_buffer->size()) {
          log()->error("selectOneOfCoro(): invalid state");
          co_return ProtocolMuxer::Error::INTERNAL_ERROR;
        }

        BytesIn data_span(*read_buffer);
        data_span = data_span.first(bytes_read);

        auto state = parser_.consume(data_span);
        if (state == Parser::kOverflow) {
          SL_TRACE(log(), "peer error: parser overflow");
          co_return ProtocolMuxer::Error::PROTOCOL_VIOLATION;
        }
        if (state != Parser::kReady) {
          continue;  // Need more data
        }

        // Process the received messages
        for (const auto &msg : parser_.messages()) {
          switch (msg.type) {
            case Message::kProtocolName: {
              // Process protocol proposal/acceptance
              auto result =
                  co_await processProtocolMessageCoro(connection,
                                                      is_initiator,
                                                      multistream_negotiated,
                                                      wait_for_protocol_reply,
                                                      current_protocol,
                                                      wait_for_reply_sent,
                                                      local_protocols,
                                                      msg,
                                                      na_response);

              // If we got a protocol or an error, return it
              if (result) {
                co_return result;
              }
              break;
            }
            case Message::kRightProtocolVersion:
              multistream_negotiated = true;
              break;
            case Message::kNAMessage: {
              // Handle NA
              if (is_initiator) {
                if (current_protocol < local_protocols.size()) {
                  SL_DEBUG(log(),
                           "protocol {} was not accepted by peer",
                           local_protocols[current_protocol]);
                }

                // Try the next protocol
                ++current_protocol;

                if (current_protocol < local_protocols.size()) {
                  auto result = co_await sendProtocolProposalCoro(
                      connection,
                      multistream_negotiated,
                      local_protocols[current_protocol]);
                  if (!result) {
                    co_return result.error();
                  }
                  wait_for_protocol_reply = true;
                } else {
                  // No more protocols to propose
                  SL_DEBUG(log(),
                           "Failed to negotiate protocols: {}",
                           fmt::join(local_protocols, ", "));
                  co_return ProtocolMuxer::Error::NEGOTIATION_FAILED;
                }
              } else {
                // Server side
                SL_DEBUG(log(), "Unexpected NA received by server");
                co_return ProtocolMuxer::Error::PROTOCOL_VIOLATION;
              }
              break;
            }
            case Message::kWrongProtocolVersion:
              co_return ProtocolMuxer::Error::PROTOCOL_VIOLATION;
              break;
            default:
              co_return ProtocolMuxer::Error::PROTOCOL_VIOLATION;
              break;
          }
        }

        // Reset parser for next messages
        parser_.reset();
      } catch (const std::exception &e) {
        log()->error("Error reading data: {}", e.what());
        co_return std::make_error_code(std::errc::io_error);
      }
    }

    // This should not be reached in normal operation
    co_return ProtocolMuxer::Error::INTERNAL_ERROR;
  }

  boost::asio::awaitable<outcome::result<void>>
  MultiselectInstance::sendProtocolProposalCoro(
      std::shared_ptr<basic::ReadWriter> connection,
      bool multistream_negotiated,
      const std::string &protocol) {
    // Create the protocol proposal message based on negotiation state
    outcome::result<MsgBuf> msg_res =
        outcome::failure(std::make_error_code(std::errc::invalid_argument));
    if (!multistream_negotiated) {
      std::array<std::string_view, 2> a({kProtocolId, protocol});
      msg_res = detail::createMessage(a, false);
    } else {
      msg_res = detail::createMessage(protocol);
    }

    if (!msg_res) {
      co_return msg_res.error();
    }

    // Send the message
    auto packet = std::make_shared<MsgBuf>(msg_res.value());
    try {
      auto ec =
          co_await connection->writeSome(BytesIn(*packet), packet->size());
      if (ec) {
        co_return ec;
      }
    } catch (const std::exception &e) {
      log()->error("Error writing protocol proposal: {}", e.what());
      co_return std::make_error_code(std::errc::io_error);
    }

    co_return outcome::success();
  }

  boost::asio::awaitable<outcome::result<peer::ProtocolName>>
  MultiselectInstance::processProtocolMessageCoro(
      std::shared_ptr<basic::ReadWriter> connection,
      bool is_initiator,
      bool multistream_negotiated,
      bool wait_for_protocol_reply,
      size_t current_protocol,
      boost::optional<size_t> &wait_for_reply_sent,
      const boost::container::small_vector<std::string, 4> &local_protocols,
      const Message &msg,
      boost::optional<std::shared_ptr<MsgBuf>> &na_response) {
    // Handle protocol name message
    if (is_initiator) {
      // Client side
      if (wait_for_protocol_reply) {
        if (current_protocol < local_protocols.size()
            && local_protocols[current_protocol] == msg.content) {
          // Successful client side negotiation
          co_return std::string(msg.content);
        }
      }
      co_return ProtocolMuxer::Error::PROTOCOL_VIOLATION;
    }

    // Server side
    size_t idx = 0;
    for (const auto &p : local_protocols) {
      if (p == msg.content) {
        // Successful server side negotiation
        wait_for_reply_sent = idx;

        // Send protocol acceptance
        auto accept_msg = detail::createMessage(msg.content);
        if (!accept_msg) {
          co_return accept_msg.error();
        }

        auto packet = std::make_shared<MsgBuf>(accept_msg.value());
        try {
          auto ec =
              co_await connection->writeSome(BytesIn(*packet), packet->size());
          if (ec) {
            co_return ec;
          }
          // Protocol negotiation successful
          co_return local_protocols[wait_for_reply_sent.value()];
        } catch (const std::exception &e) {
          log()->error("Error writing protocol acceptance: {}", e.what());
          co_return std::make_error_code(std::errc::io_error);
        }
      }
      ++idx;
    }

    // Not found, send NA
    SL_DEBUG(log(), "unknown protocol {} proposed by client", msg.content);
    if (!na_response) {
      auto na_msg = detail::createMessage(kNA);
      if (!na_msg) {
        co_return na_msg.error();
      }
      na_response = std::make_shared<MsgBuf>(na_msg.value());
    }

    try {
      auto ec = co_await connection->writeSome(BytesIn(*na_response.value()),
                                               na_response.value()->size());
      if (ec) {
        co_return ec;
      }
      // NA sent successfully, continue with protocol negotiation
      co_return outcome::success();
    } catch (const std::exception &e) {
      log()->error("Error sending NA response: {}", e.what());
      co_return std::make_error_code(std::errc::io_error);
    }
  }

}  // namespace libp2p::protocol_muxer::multiselect
