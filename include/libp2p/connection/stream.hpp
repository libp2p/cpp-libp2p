/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_STREAM_HPP
#define LIBP2P_CONNECTION_STREAM_HPP

#include <libp2p/basic/readwriter.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::peer {
  class PeerId;
}

namespace libp2p::connection {

  /**
   * Stream over some connection, allowing to write/read to/from that connection
   * @note the user MUST WAIT for the completion of method from this list
   * before calling another method from this list:
   *    - write
   *    - writeSome
   *    - close
   *    - adjustWindowSize
   *    - reset
   * Also, 'read' & 'readSome' are in another tuple. This behaviour results in
   * possibility to read and write simultaneously, but double read or write is
   * forbidden
   */
  struct Stream : public basic::ReadWriter {
    enum class Error {
      STREAM_INTERNAL_ERROR = 1,
      STREAM_INVALID_ARGUMENT,
      STREAM_PROTOCOL_ERROR,
      STREAM_IS_READING,
      STREAM_NOT_READABLE,
      STREAM_NOT_WRITABLE,
      STREAM_CLOSED_BY_HOST,
      STREAM_CLOSED_BY_PEER,
      STREAM_RESET_BY_HOST,
      STREAM_RESET_BY_PEER,
      STREAM_INVALID_WINDOW_SIZE,
      STREAM_WRITE_OVERFLOW,
      STREAM_RECEIVE_OVERFLOW,
    };

    using Handler = void(std::shared_ptr<Stream>);
    using VoidResultHandlerFunc = std::function<void(outcome::result<void>)>;

    ~Stream() override = default;

    /**
     * Check, if this stream is closed from this side of the connection and
     * thus cannot be read from
     * @return true, if stream cannot be read from, false otherwise
     */
    virtual bool isClosedForRead() const = 0;

    /**
     * Check, if this stream is closed from the other side of the connection and
     * thus cannot be written to
     * @return true, if stream cannot be written to, false otherwise
     */
    virtual bool isClosedForWrite() const = 0;

    /**
     * Check, if this stream is closed bor both writes and reads
     * @return true, if stream is closed entirely, false otherwise
     */
    virtual bool isClosed() const = 0;

    /**
     * Close a stream, indicating we are not going to write to it anymore; the
     * other side, however, can write to it, if it was not closed from there
     * before
     * @param cb to be called, when the stream is closed, or error happens
     */
    virtual void close(VoidResultHandlerFunc cb) = 0;

    /**
     * @brief Close this stream entirely; this normally means an error happened,
     * so it should not be used just to close the stream
     */
    virtual void reset() = 0;

    /**
     * Set a new receive window size of this stream - how much unread bytes can
     * we have on our side of the stream
     * @param new_size for the window
     * @param cb to be called, when the operation succeeds of fails
     */
    virtual void adjustWindowSize(uint32_t new_size,
                                  VoidResultHandlerFunc cb) = 0;

    /**
     * Is that stream opened over a connection, which was an initiator?
     */
    virtual outcome::result<bool> isInitiator() const = 0;

    /**
     * Get a peer, which the stream is connected to
     * @return id of the peer
     */
    virtual outcome::result<peer::PeerId> remotePeerId() const = 0;

    /**
     * Get a local multiaddress
     * @return address or error
     */
    virtual outcome::result<multi::Multiaddress> localMultiaddr() const = 0;

    /**
     * Get a multiaddress, to which the stream is connected
     * @return multiaddress or error
     */
    virtual outcome::result<multi::Multiaddress> remoteMultiaddr() const = 0;
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, Stream::Error)

#endif  // LIBP2P_CONNECTION_STREAM_HPP
