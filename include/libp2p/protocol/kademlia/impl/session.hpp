/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <functional>

#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::basic {
  class MessageReadWriterUvarint;
  class Scheduler;
}  // namespace libp2p::basic

namespace libp2p::protocol::kademlia {
  class ResponseHandler;
  class SessionHost;
}  // namespace libp2p::protocol::kademlia

namespace libp2p::protocol::kademlia {
  class Session : public std::enable_shared_from_this<Session> {
   public:
    Session(std::weak_ptr<basic::Scheduler> scheduler,
            std::shared_ptr<connection::Stream> stream,
            Time operations_timeout);
    ~Session();

    using OnRead = std::function<void(outcome::result<Message>)>;
    void read(OnRead on_read);
    using OnWrite = std::function<void(outcome::result<void>)>;
    void write(BytesIn frame, OnWrite on_write);

    void read(std::weak_ptr<SessionHost> weak_session_host);
    void read(std::shared_ptr<ResponseHandler> response_handler);
    void write(const Message &msg,
               std::weak_ptr<SessionHost> weak_session_host);
    void write(BytesIn frame,
               std::shared_ptr<ResponseHandler> response_handler);
    void write(BytesIn frame);

    std::shared_ptr<connection::Stream> stream() const {
      return stream_;
    }

   private:
    void setTimer();

    std::weak_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<connection::Stream> stream_;
    const Time operations_timeout_;

    std::shared_ptr<basic::MessageReadWriterUvarint> framing_;
    Cancel timer_;
  };
}  // namespace libp2p::protocol::kademlia
