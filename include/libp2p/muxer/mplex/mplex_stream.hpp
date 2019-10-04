/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MPLEX_STREAM_HPP
#define LIBP2P_MPLEX_STREAM_HPP

#include <boost/asio/streambuf.hpp>
#include <boost/noncopyable.hpp>
#include <libp2p/common/logger.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/muxer/mplex/mplexed_connection.hpp>

namespace libp2p::connection {
  /**
   * Stream implementation, used by Mplex multiplexer
   */
  class MplexStream : public Stream,
                      public std::enable_shared_from_this<MplexStream>,
                      private boost::noncopyable {
   public:
    ~MplexStream() override = default;

    /**
     * Create an instance of Mplex stream
     * @param connection, over which this stream is opened
     * @param stream_id of this stream
     */
    MplexStream(std::weak_ptr<MplexedConnection> connection,
                MplexedConnection::StreamId stream_id);

    enum class Error {
      CONNECTION_IS_DEAD = 1,
      BAD_WINDOW_SIZE,
      INVALID_ARGUMENT,
      IS_READING,
      IS_CLOSED_FOR_READS,
      IS_WRITING,
      IS_CLOSED_FOR_WRITES,
      IS_RESET,
      RECEIVE_OVERFLOW,
      INTERNAL_ERROR
    };

    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

    bool isClosed() const noexcept override;

    void close(VoidResultHandlerFunc cb) override;

    bool isClosedForRead() const noexcept override;

    bool isClosedForWrite() const noexcept override;

    void reset() override;

    void adjustWindowSize(uint32_t new_size, VoidResultHandlerFunc cb) override;

    outcome::result<peer::PeerId> remotePeerId() const override;

    outcome::result<bool> isInitiator() const override;

    outcome::result<multi::Multiaddress> localMultiaddr() const override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() const override;

   private:
    /**
     * Internal proxy method for reads; (\param some) denotes if the read should
     * read 'some' or 'all' bytes
     */
    void read(gsl::span<uint8_t> out, size_t bytes, ReadCallbackFunc cb,
              bool some);

    /**
     * Internal proxy method for writes; (\param some) denotes if the write
     * should write 'some' or 'all' bytes
     */
    void write(gsl::span<const uint8_t> in, size_t bytes, WriteCallbackFunc cb,
               bool some);

    std::weak_ptr<MplexedConnection> connection_;
    MplexedConnection::StreamId stream_id_;
    common::Logger log_ = common::createLogger("MplexStream");

    /// data, received for this stream, comes here
    boost::asio::streambuf read_buffer_;

    /// when a new data arrives, this function is to be called
    std::function<bool()> data_notifyee_;

    /// is the stream opened for reads?
    bool is_readable_ = true;
    bool is_reading_ = false;

    /// is the stream opened for writes?
    bool is_writable_ = true;
    bool is_writing_ = false;

    /// was the stream reset?
    bool is_reset_ = false;

    /// how much unread data can be in this stream at one time; if new data
    /// exceeding this value is received, the stream is reset
    uint32_t receive_window_size_ = 256 * 1024;  // 256 MB

    /// MplexedConnection API starts here
    friend class MplexedConnection;

    /**
     * Called by underlying connection to pass data, which arrived for this
     * stream
     * @param data received
     * @param data_size - size of the received data
     */
    outcome::result<void> commitData(gsl::span<const uint8_t> data,
                                     size_t data_size);
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, MplexStream::Error)

#endif  // LIBP2P_MPLEX_STREAM_HPP
