/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_YAMUX_STREAM_HPP
#define LIBP2P_YAMUX_STREAM_HPP

#include <deque>
#include <mutex>

#include <boost/asio/streambuf.hpp>
#include <boost/noncopyable.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

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

    /// this stream's connection
    std::weak_ptr<YamuxedConnection> yamuxed_connection_;

    /// id of this stream
    YamuxedConnection::StreamId stream_id_;

    /// is the stream opened for reads?
    bool is_readable_ = true;

    /// is the stream opened for writes?
    bool is_writable_ = true;

    /**
     * default sliding window size of the stream - how much unread bytes can be
     * on both sides
     */
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

    /// read callback, non-zero during async data receive
    ReadCallbackFunc read_cb_;

    /// client's read buffer
    gsl::span<uint8_t> external_read_buffer_;

    /// bytes count client is waiting for, non-zero during async data receive
    size_t bytes_waiting_ = 0;

    /// client makes readSome operation
    bool reading_some_ = false;

    /// starts async read operation
    void beginRead(ReadCallbackFunc cb, gsl::span<uint8_t> out, size_t bytes,
                   bool some);

    /// ends async read operation
    void endRead(outcome::result<size_t> result);

    /// Tries to consume requested bytes from already received data
    outcome::result<size_t> tryConsumeReadBuffer(gsl::span<uint8_t> out,
                                                 size_t bytes, bool some);

    /**
     * Forwards read buffer and receive window and acknowledges bytes received
     * in async manner
     * @param bytes number of bytes to ack
     */
    void sendAck(size_t bytes);

    /// is the stream writing right now?
    bool is_writing_ = false;

    /// write callback, non-zero during async sends
    WriteCallbackFunc write_cb_;

    /// Queue of write requests that were received when stream was writing
    std::deque<
        std::tuple<std::vector<uint8_t>, size_t, WriteCallbackFunc, bool>>
        write_queue_{};

    mutable std::mutex write_queue_mutex_;

    /// starts async write operation
    void beginWrite(WriteCallbackFunc cb);

    /// ends async write operation
    void endWrite(outcome::result<size_t> result);

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

    /// Called by connection on reset
    void onConnectionReset(outcome::result<size_t> reason);
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, YamuxStream::Error)

#endif  // LIBP2P_YAMUX_STREAM_HPP
