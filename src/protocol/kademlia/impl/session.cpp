/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/session.hpp>

#include <qtils/bytes.hpp>

#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/response_handler.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>

namespace libp2p::protocol::kademlia {
  Session::Session(std::weak_ptr<basic::Scheduler> scheduler,
                   std::shared_ptr<connection::Stream> stream,
                   Time operations_timeout)
      : scheduler_{std::move(scheduler)},
        stream_{std::move(stream)},
        operations_timeout_{operations_timeout},
        framing_{std::make_shared<basic::MessageReadWriterUvarint>(stream_)} {}

  Session::~Session() {
    stream_->reset();
  }

  void Session::read(OnRead on_read) {
    setTimer();
    framing_->read([self{shared_from_this()}, on_read{std::move(on_read)}](
                       basic::MessageReadWriter::ReadCallback r) {
      self->timer_.reset();
      if (not r) {
        on_read(r.error());
        return;
      }
      Message msg;
      if (not msg.deserialize(*r.value())) {
        on_read(Error::MESSAGE_DESERIALIZE_ERROR);
        return;
      }
      on_read(std::move(msg));
    });
  }

  void Session::write(BytesIn frame, OnWrite on_write) {
    setTimer();
    auto buf = std::make_shared<Bytes>(qtils::asVec(frame));
    libp2p::write(stream_,
                  frame,
                  [self{shared_from_this()},
                   on_write{std::move(on_write)},
                   buf](outcome::result<void> r) {
                    self->timer_.reset();
                    if (not r) {
                      on_write(r.error());
                      return;
                    }
                    on_write(outcome::success());
                  });
  }

  void Session::read(std::weak_ptr<SessionHost> weak_session_host) {
    read([self{shared_from_this()},
          weak_session_host{std::move(weak_session_host)}](
             outcome::result<Message> r) {
      auto session_host = weak_session_host.lock();
      if (not session_host) {
        return;
      }
      if (not r) {
        return;
      }
      session_host->onMessage(self, std::move(r.value()));
    });
  }

  void Session::read(std::shared_ptr<ResponseHandler> response_handler) {
    read([self{shared_from_this()},
          response_handler](outcome::result<Message> r) {
      if (r and not response_handler->match(r.value())) {
        r = Error::UNEXPECTED_MESSAGE_TYPE;
      }
      response_handler->onResult(self, std::move(r));
    });
  }

  void Session::write(const Message &msg,
                      std::weak_ptr<SessionHost> weak_session_host) {
    Bytes pb;
    if (not msg.serialize(pb)) {
      return;
    }
    write(pb,
          [self{shared_from_this()},
           weak_session_host{std::move(weak_session_host)}](
              outcome::result<void> r) {
            if (not r) {
              return;
            }
            self->read(weak_session_host);
          });
  }

  void Session::write(BytesIn frame,
                      std::shared_ptr<ResponseHandler> response_handler) {
    write(
        frame,
        [self{shared_from_this()}, response_handler](outcome::result<void> r) {
          if (not r) {
            response_handler->onResult(self, r.error());
            return;
          }
          self->read(std::move(response_handler));
        });
  }

  void Session::write(BytesIn frame) {
    write(frame, [self{shared_from_this()}](outcome::result<void> r) {});
  }

  void Session::setTimer() {
    if (operations_timeout_ == Time::zero()) {
      return;
    }
    auto scheduler = scheduler_.lock();
    if (not scheduler) {
      return;
    }
    timer_ = scheduler->scheduleWithHandle(
        [weak_self = weak_from_this()] {
          auto self = weak_self.lock();
          if (not self) {
            return;
          }
          self->stream_->reset();
        },
        operations_timeout_);
  }
}  // namespace libp2p::protocol::kademlia
