/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_PROTOCOL_SESSION_HPP
#define LIBP2P_KAD_PROTOCOL_SESSION_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/kademlia/impl/kad_session_host.hpp>

namespace libp2p::protocol::kademlia {

  class KadProtocolSession
      : public std::enable_shared_from_this<KadProtocolSession> {
   public:
    using Ptr = std::shared_ptr<KadProtocolSession>;

    using Buffer = std::shared_ptr<std::vector<uint8_t>>;

    static const int CLOSED_STATE = 0;

    KadProtocolSession(std::weak_ptr<KadSessionHost> host,
                       std::shared_ptr<connection::Stream> stream,
                       scheduler::Ticks operations_timeout=0);

    bool read();

    bool write(const Message &msg);

    bool write(Buffer buffer);

    // state is impl defined
    int state() {
      return state_;
    }
    void state(int new_state) {
      state_ = new_state;
    }

    void close();

   private:
    void onLengthRead(boost::optional<multi::UVarint> varint_opt);

    void onMessageRead(outcome::result<size_t> res);

    void onMessageWritten(outcome::result<size_t> res);

    void setTimeout();
    void cancelTimeout();

    std::weak_ptr<KadSessionHost> host_;
    std::shared_ptr<connection::Stream> stream_;
    Buffer buffer_;

    // true if msg length is read and waiting for message body
    bool reading_ = false;

    // session-defined state
    int state_ = CLOSED_STATE;

    const scheduler::Ticks operations_timeout_;
    Scheduler::Handle timeout_handle_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_PROTOCOL_SESSION_HPP
