/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_SESSION
#define LIBP2P_PROTOCOL_KADEMLIA_SESSION

#include <functional>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/response_handler.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>

namespace libp2p::protocol::kademlia {

  class FindPeerExecutor;

  class Session : public std::enable_shared_from_this<Session> {
   public:
    Session(std::weak_ptr<SessionHost> session_host,
            std::weak_ptr<basic::Scheduler> scheduler,
            std::shared_ptr<connection::Stream> stream,
            Time operations_timeout = Time::zero());

    ~Session();

    std::shared_ptr<connection::Stream> stream() const {
      return stream_;
    }

    boost::optional<Message::Type> expectedResponseType() const {
      return expected_response_type_;
    }

    bool read();

    bool write(const std::shared_ptr<std::vector<uint8_t>> &buffer,
               const std::shared_ptr<ResponseHandler> &response_handler);

    bool canBeClosed() const {
      return not closed_ && writing_ == 0 && reading_ == 0;
    }

    void close(outcome::result<void> = outcome::success());

   private:
    void onLengthRead(outcome::result<multi::UVarint> varint);

    void onMessageRead(outcome::result<size_t> res);

    void onMessageWritten(
        outcome::result<size_t> res,
        const std::shared_ptr<ResponseHandler> &response_handler);

    void setReadingTimeout();
    void cancelReadingTimeout();

    void setResponseTimeout(
        const std::shared_ptr<ResponseHandler> &response_handler);
    void cancelResponseTimeout(
        const std::shared_ptr<ResponseHandler> &response_handler);

    std::unordered_map<std::shared_ptr<ResponseHandler>,
                       basic::Scheduler::Handle>
        response_handlers_;

    std::weak_ptr<SessionHost> session_host_;
    std::weak_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<connection::Stream> stream_;

    std::vector<uint8_t> inner_buffer_;

    std::atomic_size_t reading_ = 0;
    std::atomic_size_t writing_ = 0;
    bool closed_ = false;

    const Time operations_timeout_;
    basic::Scheduler::Handle reading_timeout_handle_;

    boost::optional<Message::Type> expected_response_type_;

    static std::atomic_size_t instance_number;
    log::SubLogger log_;
  };
}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_SESSION
