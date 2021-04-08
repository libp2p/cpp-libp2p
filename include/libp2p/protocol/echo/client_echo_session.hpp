/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CLIENT_ECHO_SESSION_HPP
#define LIBP2P_CLIENT_ECHO_SESSION_HPP

#include <vector>

#include <libp2p/connection/stream.hpp>

namespace libp2p::protocol {

  /**
   * @brief Session, created by client. Basically, a convenient interface to
   * echo server.
   */
  class ClientEchoSession
      : public std::enable_shared_from_this<ClientEchoSession> {
   public:
    using Then = std::function<void(outcome::result<std::string>)>;

    explicit ClientEchoSession(std::shared_ptr<connection::Stream> stream);

    /**
     * @brief Send a message, read back the same message and execute {@param
     * then} with that message.
     */
    void sendAnd(const std::string &send, Then then);

   private:
    void doRead();
    void completed();

    std::shared_ptr<connection::Stream> stream_;
    std::vector<uint8_t> buf_;
    std::vector<uint8_t> recv_buf_;
    std::error_code ec_;
    size_t bytes_read_ = 0;
    Then then_;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_CLIENT_ECHO_SESSION_HPP
