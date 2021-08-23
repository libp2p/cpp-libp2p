/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/session.hpp>

#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/find_peer_executor.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  std::atomic_size_t Session::instance_number = 0;

  Session::Session(std::weak_ptr<SessionHost> session_host,
                   std::weak_ptr<basic::Scheduler> scheduler,
                   std::shared_ptr<connection::Stream> stream,
                   Time operations_timeout)
      : session_host_(std::move(session_host)),
        scheduler_(std::move(scheduler)),
        stream_(std::move(stream)),
        operations_timeout_(operations_timeout),
        log_("KademliaSession", "kademlia", "Session", ++instance_number) {
    log_.debug("created");
  }

  Session::~Session() {
    log_.debug("destroyed");
  }

  bool Session::read() {
    if (stream_->isClosedForRead()) {
      close(Error::STREAM_RESET);
      return false;
    }

    ++reading_;

    libp2p::basic::VarintReader::readVarint(
        stream_,
        [wp = weak_from_this()](outcome::result<multi::UVarint> varint) {
          if (auto self = wp.lock())
            self->onLengthRead(std::move(varint));
        });
    setReadingTimeout();
    return true;
  }

  bool Session::write(
      const std::shared_ptr<std::vector<uint8_t>> &buffer,
      const std::shared_ptr<ResponseHandler> &response_handler) {
    if (stream_->isClosedForWrite()) {
      close(Error::STREAM_RESET);
      return false;
    }

    ++writing_;

    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    stream_->write(gsl::span(buffer->data(), buffer->size()), buffer->size(),
                   [wp = weak_from_this(), buffer,
                    response_handler](outcome::result<size_t> result) {
                     if (auto self = wp.lock()) {
                       self->onMessageWritten(result, response_handler);
                     }
                   });

    setResponseTimeout(response_handler);
    return true;
  }

  void Session::close(outcome::result<void> reason) {
    if (closed_) {
      return;
    }

    closed_ = true;

    if (reason.has_value()) {
      reason = Error::SESSION_CLOSED;
    }

    for (auto &pair : response_handlers_) {
      pair.second.cancel();
      pair.first->onResult(shared_from_this(), reason.as_failure());
    }
    response_handlers_.clear();

    cancelReadingTimeout();

    stream_->close([self{shared_from_this()}](outcome::result<void>) {});

    if (auto session_host = session_host_.lock()) {
      session_host->closeSession(stream_);
    }
  }

  void Session::onLengthRead(outcome::result<multi::UVarint> varint) {
    if (stream_->isClosedForRead()) {
      close(Error::STREAM_RESET);
      return;
    }

    if (closed_) {
      return;
    }

    if (varint.has_error()) {
      close(varint.error());
      return;
    }

    auto msg_len = varint.value().toUInt64();
    inner_buffer_.resize(msg_len);

    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    stream_->read(gsl::span(inner_buffer_.data(), inner_buffer_.size()),
                  msg_len, [wp = weak_from_this()](auto &&res) {
                    if (auto self = wp.lock()) {
                      self->onMessageRead(std::forward<decltype(res)>(res));
                    }
                  });
  }

  void Session::onMessageRead(outcome::result<size_t> res) {
    cancelReadingTimeout();

    if (closed_) {
      return;
    }

    if (not res) {
      close(res.as_failure());
      return;
    }

    if (inner_buffer_.size() != res.value()) {
      close(Error::MESSAGE_PARSE_ERROR);
      return;
    }

    Message msg;
    if (!msg.deserialize(inner_buffer_.data(), inner_buffer_.size())) {
      close(Error::MESSAGE_DESERIALIZE_ERROR);
      return;
    }

    switch (msg.type) {
      case Message::Type::kPutValue:
        log_.debug("Incoming PutValue message");
        break;
      case Message::Type::kGetValue:
        log_.debug("Incoming GetValue message");
        break;
      case Message::Type::kAddProvider:
        log_.debug("Incoming AddProvider message");
        break;
      case Message::Type::kGetProviders:
        log_.debug("Incoming GetProviders message");
        break;
      case Message::Type::kFindNode:
        log_.debug("Incoming FindNode message");
        break;
      case Message::Type::kPing:
        log_.debug("Incoming Ping message");
        break;
      default:
        close(Error::UNEXPECTED_MESSAGE_TYPE);
        return;
    }

    --reading_;

    bool pocessed = false;
    for (auto it = response_handlers_.begin();
         it != response_handlers_.end();) {
      auto cit = it++;
      auto &[response_handler, timeout_handle] = *cit;
      if (response_handler->match(msg)) {
        timeout_handle.cancel();
        response_handler->onResult(shared_from_this(), msg);
        response_handlers_.erase(cit);
        pocessed = true;
      }
    }

    // Propogate to session host
    if (not pocessed) {
      if (auto session_host = session_host_.lock()) {
        session_host->onMessage(shared_from_this(), std::move(msg));
      }
    }

    // Continue to wait some response
    if (not response_handlers_.empty()) {
      read();
    }

    if (canBeClosed()) {
      close();
    }
  }

  void Session::onMessageWritten(
      outcome::result<size_t> res,
      const std::shared_ptr<ResponseHandler> &response_handler) {
    if (not res) {
      close(res.as_failure());
      return;
    }

    --writing_;

    if (not response_handlers_.empty()) {
      read();
    }

    if (canBeClosed()) {
      close();
    }
  }

  void Session::setReadingTimeout() {
    if (operations_timeout_ == Time::zero()) {
      return;
    }

    auto scheduler = scheduler_.lock();
    if (not scheduler) {
      close(Error::INTERNAL_ERROR);
      return;
    }

    reading_timeout_handle_ = scheduler->scheduleWithHandle(
        [wp = weak_from_this()] {
          if (auto self = wp.lock()) {
            self->close(Error::TIMEOUT);
          }
        },
        operations_timeout_);
  }

  void Session::cancelReadingTimeout() {
    reading_timeout_handle_.cancel();
  }

  void Session::setResponseTimeout(
      const std::shared_ptr<ResponseHandler> &response_handler) {
    if (not response_handler) {
      return;
    }

    if (response_handler->responseTimeout() == Time::zero()) {
      return;
    }

    auto scheduler = scheduler_.lock();
    if (not scheduler) {
      close(Error::INTERNAL_ERROR);
      return;
    }

    response_handlers_.emplace(
        response_handler,
        scheduler->scheduleWithHandle(
            [wp = weak_from_this(), response_handler] {
              if (auto self = wp.lock()) {
                if (response_handler) {
                  self->cancelResponseTimeout(response_handler);
                  response_handler->onResult(self, Error::TIMEOUT);
                  self->close();
                }
              }
            },
            response_handler->responseTimeout()));
  }

  void Session::cancelResponseTimeout(
      const std::shared_ptr<ResponseHandler> &response_handler) {
    if (not response_handler) {
      return;
    }

    if (auto it = response_handlers_.find(response_handler);
        it != response_handlers_.end()) {
      it->second.cancel();
      response_handlers_.erase(it);
    }
  }

}  // namespace libp2p::protocol::kademlia
