/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MPLEXED_CONNECTION_HPP
#define LIBP2P_MPLEXED_CONNECTION_HPP

#include <unordered_map>
#include <utility>

#include <libp2p/common/logger.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>

namespace libp2p::connection {
  class MplexStream;
  struct MplexFrame;

  class MplexedConnection
      : public CapableConnection,
        public std::enable_shared_from_this<MplexedConnection> {
   public:
    using StreamId = uint32_t;

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
    /**
     * Write (\param bytes) to the connection
     */
    void write(common::ByteArray bytes);

    /**
     * Read next frame from the connection
     */
    void readNextFrame();

    /**
     * Process a received (\param frame)
     */
    void processFrame(MplexFrame frame);

    /**
     * Process a new stream (\package frame)
     */
    void processNewStreamFrame(const MplexFrame &frame);

    /**
     * Process a message stream (\package frame)
     */
    void processMessageFrame(const MplexFrame &frame);

    /**
     * Process a close stream (\package frame)
     */
    void processCloseFrame(const MplexFrame &frame);

    /**
     * Process a reset stream (\package frame)
     */
    void processResetFrame(const MplexFrame &frame);

    /**
     * Send a reset over all streams over this connection
     */
    void resetAllStreams() const;

    /**
     * Close this Mplex session and the underlying connection
     */
    void closeSession();

    std::shared_ptr<SecureConnection> connection_;
    muxer::MuxedConnectionConfig config_;

    /// two streams can hold the same StreamId, so they are differentiated also
    /// by the side, who opened them
    std::unordered_map<std::pair<StreamId, bool>, MplexStream> streams_;
    StreamId last_issued_stream_id_ = 1;
    NewStreamHandlerFunc new_stream_handler_;

    bool is_active_ = false;
    common::Logger log_ = common::createLogger("MplexedConnection");

    /// MPLEX STREAM API

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
     * Add a handler function, which is called, when data for a particular
     * stream is received
     * @param stream_id of the stream which is to be notified
     * @param handler to be called; if it returns true, it's removed from
     * the list of handlers for that stream
     * @note this is done through a function and not event emitters, as each
     * stream is to receive that event independently based on id
     */
    void streamOnAddData(StreamId stream_id, NotifyeeCallback cb);
    std::map<StreamId, NotifyeeCallback> data_subs_;

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

#endif  // LIBP2P_MPLEXED_CONNECTION_HPP
