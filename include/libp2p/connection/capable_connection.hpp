/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CAPABLE_CONNECTION_HPP
#define LIBP2P_CAPABLE_CONNECTION_HPP

#include <functional>

#include <libp2p/connection/secure_connection.hpp>

namespace libp2p::connection {
  struct Stream;

  /**
   * Connection that provides basic libp2p requirements to the connection: it is
   * both secured and muxed (streams can be created over that connection)
   */
  struct CapableConnection : public SecureConnection {
    using StreamHandler = void(outcome::result<std::shared_ptr<Stream>>);
    using StreamHandlerFunc = std::function<StreamHandler>;

    using NewStreamHandlerFunc = std::function<void(std::shared_ptr<Stream>)>;

    using ConnectionClosedCallback = std::function<void(
        const peer::PeerId &,
        const std::shared_ptr<connection::CapableConnection> &)>;

    ~CapableConnection() override = default;

    /**
     * Start to process incoming messages for this connection
     * @note non-blocking
     *
     * @note make sure onStream(..) was called, so that new streams are accepted
     * by this connection - call to start() will fail otherwise
     */
    virtual void start() = 0;

    /**
     * Stop processing incoming messages for this connection without closing the
     * connection itself
     * @note calling 'start' after 'close' is UB
     */
    virtual void stop() = 0;

    /**
     * @brief Opens new stream in a synchronous (optimistic) manner
     * @return Stream or error
     */
    virtual outcome::result<std::shared_ptr<Stream>> newStream() = 0;

    /**
     * @brief Opens new stream using this connection
     * @param cb - callback to be called, when a new stream is established or
     * error appears
     */
    virtual void newStream(StreamHandlerFunc cb) = 0;

    /**
     * @brief Set a handler, which is called, when a new stream arrives from the
     * other side
     * @param cb, to which a received stream is passed
     * @note if a handler is not set, all received streams will be immediately
     * reset
     */
    virtual void onStream(NewStreamHandlerFunc cb) = 0;
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_CAPABLE_CONNECTION_HPP
