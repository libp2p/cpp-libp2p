/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TEST_ACCEPTANCE_LIBP2P_HOST_PROTOCOL_CLIENT_TEST_SESSION_HPP
#define LIBP2P_TEST_ACCEPTANCE_LIBP2P_HOST_PROTOCOL_CLIENT_TEST_SESSION_HPP

#include <vector>

#include "libp2p/connection/stream.hpp"
#include "libp2p/crypto/random_generator.hpp"

namespace libp2p::protocol {

  /**
   * @brief Session, created by client. Basically, a convenient interface to
   * echo server.
   */
  class ClientTestSession
      : public std::enable_shared_from_this<ClientTestSession> {
   public:
    using Callback = std::function<void(outcome::result<std::vector<uint8_t>>)>;

    /**
     * @param stream data stream
     * @param ping_times number of messages to be sent
     */
    ClientTestSession(std::shared_ptr<connection::Stream> stream,
                      size_t ping_times);

    /**
     * @brief Send a random message, read back the same message and execute
     * {@param cb} with that message and repeat it ping_times.
     */
    void handle(Callback cb);

    inline auto bufferSize() const {
      return buffer_size_;
    }

   private:
    void write(Callback cb);

    void read(Callback cb);

    const size_t buffer_size_ = 32u;
    std::shared_ptr<connection::Stream> stream_;
    std::shared_ptr<crypto::random::RandomGenerator> random_generator_;
    std::vector<uint8_t> write_buf_;
    std::vector<uint8_t> read_buf_;
    size_t messages_left_;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_TEST_ACCEPTANCE_LIBP2P_HOST_PROTOCOL_CLIENT_TEST_SESSION_HPP
