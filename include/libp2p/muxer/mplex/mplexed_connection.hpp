/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MPLEXED_CONNECTION_HPP
#define LIBP2P_MPLEXED_CONNECTION_HPP

#include <queue>
#include <unordered_map>
#include <utility>

#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/muxer/mplex/mplex_stream.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>

namespace libp2p::connection {
  struct MplexFrame;

  class MplexedConnection
      : public CapableConnection,
        public std::enable_shared_from_this<MplexedConnection> {
   public:
    /**
     * Create a new instance of MplexedConnection
     * @param connection to be multiplexed
     * @param config of the multiplexer
     */
    MplexedConnection(std::shared_ptr<SecureConnection> connection,
                      muxer::MuxedConnectionConfig config);

    MplexedConnection(const MplexedConnection &other) = delete;
    MplexedConnection &operator=(const MplexedConnection &other) = delete;
    MplexedConnection(MplexedConnection &&other) noexcept = delete;
    MplexedConnection &operator=(MplexedConnection &&other) noexcept = delete;
    ~MplexedConnection() override = default;

    void start() override;

    void stop() override;

    outcome::result<std::shared_ptr<Stream>> newStream() override;

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

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

   private:
    struct WriteData {
      common::ByteArray data;
      WriteCallbackFunc cb;
    };
    std::queue<WriteData> write_queue_;
    bool is_writing_ = false;

    /**
     * Write (\param data) to the connection
     */
    void write(WriteData data);

    /**
     * Write next message from the queue
     */
    void doWrite();

    /**
     * Called, when write is complete
     */
    void onWriteCompleted(outcome::result<size_t> write_res);

    /**
     * Read next frame from the connection
     */
    void readNextFrame();

    /**
     * Process a received (\param frame)
     */
    void processFrame(const MplexFrame &frame);

    /**
     * Process a new stream (\package frame)
     */
    void processNewStreamFrame(const MplexFrame &frame,
                               MplexStream::StreamId stream_id);

    /**
     * Process a message stream (\package frame)
     */
    void processMessageFrame(const MplexFrame &frame,
                             MplexStream::StreamId stream_id);

    /**
     * Process a close stream (\package frame)
     */
    void processCloseFrame(const MplexFrame &frame,
                           MplexStream::StreamId stream_id);

    /**
     * Process a reset stream (\package frame)
     */
    void processResetFrame(const MplexFrame &frame,
                           MplexStream::StreamId stream_id);

    /**
     * Find a stream with (\param id)
     * @return found stream or nothing
     */
    boost::optional<std::shared_ptr<MplexStream>> findStream(
        const MplexStream::StreamId &id) const;

    /**
     * Remove the stream from this connection and make it both non-readable and
     * non-writable
     */
    void removeStream(MplexStream::StreamId stream_id);

    /**
     * Send a reset to stream with (\param stream_id)
     */
    void resetStream(MplexStream::StreamId stream_id);

    /**
     * Send a reset over all streams over this connection
     */
    void resetAllStreams();

    /**
     * Close this Mplex session and the underlying connection
     */
    void closeSession();

    std::shared_ptr<SecureConnection> connection_;
    muxer::MuxedConnectionConfig config_;

    std::unordered_map<MplexStream::StreamId, std::shared_ptr<MplexStream>>
        streams_;
    MplexStream::StreamNumber last_issued_stream_number_ = 1;
    NewStreamHandlerFunc new_stream_handler_;

    bool is_active_ = false;
    log::Logger log_ = log::createLogger("MplexConn");

    /// MPLEX STREAM API
    friend class MplexStream;

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
    void streamWrite(MplexStream::StreamId stream_id,
                     gsl::span<const uint8_t> in, size_t bytes,
                     basic::Writer::WriteCallbackFunc cb);

    /**
     * Send a message, which denotes, that this stream is not going to write
     * any bytes from now on
     * @param stream_id of the stream
     * @param cb - callback to be called, when operation finishes
     */
    void streamClose(MplexStream::StreamId stream_id,
                     std::function<void(outcome::result<void>)> cb);

    /**
     * Send a message, which denotes, that this stream is not going to write
     * or read any bytes from now on
     * @param stream_id of the stream
     */
    void streamReset(MplexStream::StreamId stream_id);
  };
}  // namespace libp2p::connection

#endif  // LIBP2P_MPLEXED_CONNECTION_HPP
