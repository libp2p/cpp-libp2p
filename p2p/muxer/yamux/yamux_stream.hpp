/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_STREAM_HPP
#define KAGOME_YAMUX_STREAM_HPP

#include <boost/asio/streambuf.hpp>
#include <boost/noncopyable.hpp>
#include "connection/stream.hpp"
#include "muxer/yamux/yamuxed_connection.hpp"

namespace libp2p::connection {
  /**
   * Stream implementation, used by Yamux multiplexer
   */
  class YamuxStream : public Stream,
                      public std::enable_shared_from_this<YamuxStream>,
                      private boost::noncopyable {
   public:
    ~YamuxStream() override = default;

    /**
     * Create an instance of YamuxStream
     * @param yamuxed_connection, over which this stream is created
     * @param stream_id - id of this stream
     * @param maximum_window_size - maximum size of the stream's window
     */
    YamuxStream(std::weak_ptr<YamuxedConnection> yamuxed_connection,
                YamuxedConnection::StreamId stream_id,
                uint32_t maximum_window_size);

    enum class Error {
      NOT_WRITABLE = 1,
      NOT_READABLE,
      INVALID_ARGUMENT,
      RECEIVE_OVERFLOW,
      IS_WRITING,
      IS_READING,
      INVALID_WINDOW_SIZE,
      CONNECTION_IS_DEAD,
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

    std::weak_ptr<YamuxedConnection> yamuxed_connection_;
    YamuxedConnection::StreamId stream_id_;

    /// is the stream opened for reads?
    bool is_readable_ = true;

    /// is the stream opened for writes?
    bool is_writable_ = true;

    /// default sliding window size of the stream - how much unread bytes can be
    /// on both sides
    static constexpr uint32_t kDefaultWindowSize = 256 * 1024;  // in bytes

    /// how much unacked bytes can we have on our side
    uint32_t receive_window_size_ = kDefaultWindowSize;

    /// maximum value of 'receive_window_size_'
    uint32_t maximum_window_size_;

    /// how much unacked bytes can we have sent to the other side
    uint32_t send_window_size_ = kDefaultWindowSize;

    /// buffer with bytes, not consumed by this stream
    boost::asio::streambuf read_buffer_;

    /// is the stream reading right now?
    bool is_reading_ = false;

    /// is the stream writing right now?
    bool is_writing_ = false;

    /// YamuxedConnection API starts here
    friend class YamuxedConnection;

    /**
     * Called by underlying connection to signalize the stream was reset
     */
    void resetStream();

    /**
     * Called by underlying connection to signalize some data was received for
     * this stream
     * @param data received
     * @param data_size - size of the received data
     */
    outcome::result<void> commitData(gsl::span<const uint8_t> data,
                                     size_t data_size);
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, YamuxStream::Error)

#endif  // KAGOME_YAMUX_STREAM_HPP
