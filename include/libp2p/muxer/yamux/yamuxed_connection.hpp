/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_YAMUXED_CONNECTION_HPP
#define LIBP2P_YAMUXED_CONNECTION_HPP

#include <functional>
#include <map>
#include <queue>

#include <boost/asio/streambuf.hpp>
#include <libp2p/common/logger.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>

namespace libp2p::connection {
  struct YamuxFrame;
  class YamuxStream;

  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class YamuxedConnection
      : public CapableConnection,
        public std::enable_shared_from_this<YamuxedConnection> {
   public:
    using StreamId = uint32_t;
    using Buffer = common::ByteArray;

    enum class Error {
      NO_SUCH_STREAM = 1,
      YAMUX_IS_CLOSED,
      TOO_MANY_STREAMS,
      FORBIDDEN_CALL,
      OTHER_SIDE_ERROR,
      INTERNAL_ERROR,
      CLOSED_BY_PEER,
    };

    /**
     * Create a new YamuxedConnection instance
     * @param connection to be multiplexed by this instance
     * @param config to configure this instance
     * @param logger to output messages
     */
    explicit YamuxedConnection(std::shared_ptr<SecureConnection> connection,
                               muxer::MuxedConnectionConfig config = {});

    YamuxedConnection(const YamuxedConnection &other) = delete;
    YamuxedConnection &operator=(const YamuxedConnection &other) = delete;
    YamuxedConnection(YamuxedConnection &&other) noexcept = delete;
    YamuxedConnection &operator=(YamuxedConnection &&other) noexcept = delete;
    ~YamuxedConnection() override = default;

    void start() override;

    void stop() override;

    void newStream(StreamHandlerFunc cb) override;

    void onStream(NewStreamHandlerFunc cb) override;

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<void> close() override;

    bool isClosed() const override;

    /// usage of these four methods is highly not recommended or even forbidden:
    /// use stream over this connection instead
    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;
    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;
    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;
    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

   private:
    struct WriteData {
      Buffer data{};
      std::function<void(outcome::result<size_t>)> cb{};
      bool some = false;  // true, if writeSome is to be called over the data
    };
    std::queue<WriteData> write_queue_;
    bool is_writing_ = false;

    // indicates whether start() has been executed or not
    bool started_ = false;

    /**
     * Write message to the connection; ensures no more than one wright
     * would be executed at one time
     * @param write_data - data to be written with a callback
     */
    void write(WriteData write_data);

    /**
     * First part of writing loop, which takes queued messaged to be written
     */
    void doWrite();

    /**
     * Finishing part of writing loop
     * @param res, with which the last write finished
     */
    void writeCompleted(outcome::result<size_t> res);

    /// buffers to store header and data parts of Yamux frame, which were
    /// read last
    Buffer header_buffer_;
    Buffer data_buffer_;

    /**
     * First part of reader loop, which is going to read a header
     */
    void doReadHeader();

    /**
     * Finishing part of the reader loop
     * @param res, with which the last read finished
     */
    void readHeaderCompleted(outcome::result<size_t> res);

    /**
     * Read a data part of Yamux frame
     * @param data_size - size of the data to be read
     * @param cb - callback, which is called, when the data is read
     */
    void doReadData(size_t data_size, basic::Reader::ReadCallbackFunc cb);

    /**
     * Process frame of data or window update type
     * @param frame to be processed
     */
    void processDataOrWindowUpdateFrame(const YamuxFrame &frame);

    /**
     * Process frame of ping type
     * @param frame to be processed
     */
    void processPingFrame(const YamuxFrame &frame);

    /**
     * Process frame of go away type
     * @param frame to be processed
     */
    void processGoAwayFrame(const YamuxFrame &frame);

    /**
     * Reset all streams, which were created over this connection
     */
    void resetAllStreams(outcome::result<void> reason);

    /**
     * Find stream with such id in local streams
     * @param stream_id to be found
     * @return stream, if it is opened on this side, nullptr otherwise
     */
    std::shared_ptr<YamuxStream> findStream(StreamId stream_id);

    /**
     * Register a new stream in this instance, making it active
     * @param stream_id to be registered
     * @return pointer to a newly registered stream
     */
    std::shared_ptr<YamuxStream> registerNewStream(StreamId stream_id);

    /**
     * If there is data in this length, buffer it to the according stream
     * @param stream, for which the data arrived
     * @param frame, which can have some data inside
     * @return true if there is some data in the frame, and the function is
     * going to read it, false otherwise
     * @param discard_data - set to true, if the data is to be discarded after
     * read
     */
    void processData(std::shared_ptr<YamuxStream> stream,
                     const YamuxFrame &frame, bool discard_data);

    /**
     * Process a window update by notifying a related stream about a change
     * in window size
     * @param stream to be notified
     * @param window_delta - delta of window size (can be both positive and
     * negative)
     */
    void processWindowUpdate(const std::shared_ptr<YamuxStream> &stream,
                             uint32_t window_delta);

    /**
     * Close stream for reads on this side
     * @param stream_id to be closed
     */
    void closeStreamForRead(StreamId stream_id);

    /**
     * Close stream for writes from this side
     * @param stream_id to be closed
     * @param cb - callback to be called, when operation finishes
     */
    void closeStreamForWrite(StreamId stream_id,
                             std::function<void(outcome::result<void>)> cb);

    /**
     * Close stream entirely
     * @param stream_id to be closed
     */
    void removeStream(StreamId stream_id);

    /**
     * Get a stream id, with which a new stream is to be created
     * @return new id
     */
    StreamId getNewStreamId();

    /**
     * Close this Yamux session
     */
    void closeSession(outcome::result<void> reason);

    /// Underlying connection
    std::shared_ptr<SecureConnection> connection_;

    /// Handler for new inbound streams
    NewStreamHandlerFunc new_stream_handler_;

    /// Config constants
    muxer::MuxedConnectionConfig config_;

    /// Last stream id to be incremented
    uint32_t last_created_stream_id_;

    /// Streams
    std::unordered_map<StreamId, std::shared_ptr<YamuxStream>> streams_;

    libp2p::common::Logger log_ = libp2p::common::createLogger("yx-conn");

    /// YAMUX STREAM API

    friend class YamuxStream;

    using NotifyeeCallback = std::function<bool()>;

    /**
     * Add a handler function, which is called, when a window update is
     * received
     * @param stream_id of the stream which is to be notified
     * @param handler to be called; if it returns true, it's removed from
     * the list of handlers for that stream
     * @note this is done through a function and not event emitters, as each
     * stream is to receive that event independently based on id
     */
    void streamOnWindowUpdate(StreamId stream_id, NotifyeeCallback cb);
    std::map<StreamId, NotifyeeCallback> window_updates_subs_;

    /**
     * Write bytes to the connection; before calling this method, the stream
     * must ensure that no write operations are currently running
     * @param stream_id, for which the bytes are to be written
     * @param in - bytes to be written
     * @param bytes - number of bytes to be written
     * @param some - some or all bytes must be written
     * @param cb - callback to be called after write attempt with number of
     * bytes written or error
     */
    void streamWrite(StreamId stream_id, gsl::span<const uint8_t> in,
                     size_t bytes, bool some,
                     basic::Writer::WriteCallbackFunc cb);

    /**
     * Send an acknowledgement, that a number of bytes was consumed by the
     * stream
     * @param stream_id of the stream
     * @param bytes - number of consumed bytes
     * @param cb - callback to be called, when operation finishes
     */
    void streamAckBytes(StreamId stream_id, uint32_t bytes,
                        std::function<void(outcome::result<void>)> cb);

    /**
     * Send a message, which denotes, that this stream is not going to write
     * any bytes from now on
     * @param stream_id of the stream
     * @param cb - callback to be called, when operation finishes
     */
    void streamClose(StreamId stream_id,
                     std::function<void(outcome::result<void>)> cb);

    /**
     * Send a message, which denotes, that this stream is not going to write
     * or read any bytes from now on
     * @param stream_id of the stream
     * @param cb - callback to be called, when operation finishes
     */
    void streamReset(StreamId stream_id,
                     std::function<void(outcome::result<void>)> cb);
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, YamuxedConnection::Error)

#endif  // LIBP2P_YAMUX_IMPL_HPP
