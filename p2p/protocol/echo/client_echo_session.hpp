/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLIENT_ECHO_SESSION_HPP
#define KAGOME_CLIENT_ECHO_SESSION_HPP

#include <vector>

#include "connection/stream.hpp"

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
    std::shared_ptr<connection::Stream> stream_;
    std::vector<uint8_t> buf_;
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_CLIENT_ECHO_SESSION_HPP
