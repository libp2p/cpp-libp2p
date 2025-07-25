/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include <libp2p/basic/read_buffer.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/muxer/yamux/yamux_reading_state.hpp>
#include <libp2p/muxer/yamux/yamux_stream.hpp>

namespace libp2p::connection {

  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class YamuxedConnection final
      : public CapableConnection,
        public YamuxStreamFeedback,
        public std::enable_shared_from_this<YamuxedConnection> {
   public:
    using StreamId = uint32_t;

    YamuxedConnection(const YamuxedConnection &other) = delete;
    YamuxedConnection &operator=(const YamuxedConnection &other) = delete;
    YamuxedConnection(YamuxedConnection &&other) = delete;
    YamuxedConnection &operator=(YamuxedConnection &&other) = delete;
    ~YamuxedConnection() override = default;

    /**
     * Create a new YamuxedConnection instance
     * @param connection to be multiplexed by this instance
     * @param config to configure this instance
     * @param logger to output messages
     */
    explicit YamuxedConnection(std::shared_ptr<SecureConnection> connection,
                               std::shared_ptr<basic::Scheduler> scheduler,
                               ConnectionClosedCallback closed_callback,
                               muxer::MuxedConnectionConfig config = {});

    void start() override;

    void stop() override;

    outcome::result<std::shared_ptr<Stream>> newStream() override;

    void newStream(StreamHandlerFunc cb) override;

    void onStream(NewStreamHandlerFunc cb) override;

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<void> close() override;

    bool isClosed() const override;

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

   private:
    using Streams = std::unordered_map<StreamId, std::shared_ptr<YamuxStream>>;

    using PendingOutboundStreams =
        std::unordered_map<StreamId, StreamHandlerFunc>;

    using Buffer = Bytes;

    struct WriteQueueItem {
      // TODO(artem): reform in buffers (shared + vector writes)

      Buffer packet;
      StreamId stream_id;
    };

    // YamuxStreamFeedback interface overrides

    /// Stream transfers data to connection
    void writeStreamData(uint32_t stream_id, BytesIn data) override;

    /// Stream acknowledges received bytes
    void ackReceivedBytes(uint32_t stream_id, uint32_t bytes) override;

    /// Stream defers callback to avoid reentrancy
    void deferCall(std::function<void()>) override;

    /// Stream closes (if immediately==false then all pending data will be sent)
    void resetStream(uint32_t stream_id) override;

    void streamClosed(uint32_t stream_id) override;

    /// usage of these four methods is highly not recommended or even forbidden:
    /// use stream over this connection instead
    void readSome(BytesOut out, ReadCallbackFunc cb) override;
    void writeSome(BytesIn in, WriteCallbackFunc cb) override;

    /// Initiates async readSome on connection
    void continueReading();

    /// Read callback
    void onRead(outcome::result<size_t> res);

    /// Processes incoming header, called from YamuxReadingState
    bool processHeader(boost::optional<YamuxFrame> header);

    /// Processes incoming data, called from YamuxReadingState
    void processData(BytesOut segment, StreamId stream_id);

    /// FIN received from peer to stream (either in header or with last data
    /// segment)
    void processFin(StreamId stream_id);

    /// RST received from peer to stream (either in header or with last data
    /// segment)
    void processRst(StreamId stream_id);

    /// Processes incoming GO_AWAY frame
    void processGoAway(const YamuxFrame &frame);

    /// Processes incoming frame with SYN flag
    bool processSyn(const YamuxFrame &frame);

    /// Processes incoming frame with ACK flag
    bool processAck(const YamuxFrame &frame);

    /// Processes incoming WINDOW_UPDATE message
    bool processWindowUpdate(const YamuxFrame &frame);

    /// Closes everything, notifies streams and handlers
    void close(std::error_code notify_streams_code,
               boost::optional<YamuxFrame::GoAwayError> reply_to_peer_code);

    /// Writes data to underlying connection or (if is_writing_) enqueues them
    /// If stream_id != 0, stream will be acknowledged about data written
    void enqueue(Buffer packet, StreamId stream_id = 0);

    /// Performs write into connection
    void doWrite(WriteQueueItem packet);

    /// Write callback
    void onDataWritten(outcome::result<void> res, WriteQueueItem &&packet);

    /// Creates new yamux stream
    std::shared_ptr<Stream> createStream(StreamId stream_id);

    /// Erases stream by id, may affect incactivity timer
    void eraseStream(StreamId stream_id);

    /// Erases entry from pending streams, may affect incactivity timer
    void erasePendingOutboundStream(PendingOutboundStreams::iterator it);

    /// Sets expire timer if last stream was just closed. Called from erase*()
    /// functions
    void adjustExpireTimer();

    /// Expire timer callback
    void onExpireTimer();

    void setTimerCleanup();
    void setTimerPing();

    /// Copy of config
    const muxer::MuxedConnectionConfig config_;

    /// Underlying connection
    std::shared_ptr<SecureConnection> connection_;

    /// Scheduler
    std::shared_ptr<basic::Scheduler> scheduler_;

    /// True if started
    bool started_ = false;

    /// TODO(artem): change read() interface to reduce copying
    std::shared_ptr<Buffer> raw_read_buffer_;

    /// Buffering and segmenting
    YamuxReadingState reading_state_;

    /// True if waiting for current write operation to complete
    bool is_writing_ = false;

    std::shared_ptr<Bytes> writing_buf_ = std::make_shared<Bytes>();

    /// Write queue
    std::deque<WriteQueueItem> write_queue_;

    /// Active streams
    Streams streams_;

    /// Streams just created. Need to call handlers after all
    /// data is processed. StreamHandlerFunc is null for inbound streams
    std::vector<std::pair<StreamId, StreamHandlerFunc>> fresh_streams_;

    /// Handler for new inbound streams
    NewStreamHandlerFunc new_stream_handler_;

    /// New stream id (odd if underlying connection is outbound)
    StreamId new_stream_id_ = 0;

    /// Pending outbound streams
    PendingOutboundStreams pending_outbound_streams_;

    /// Timer handle for pings
    basic::Scheduler::Handle ping_handle_;

    /// Cleanup for detached streams
    basic::Scheduler::Handle cleanup_handle_;

    /// Timer handle for auto closing if inactive
    basic::Scheduler::Handle inactivity_handle_;

    /// Called on connection close
    ConnectionClosedCallback closed_callback_;

    /// Remote peer saved here
    peer::PeerId remote_peer_;

    uint32_t ping_counter_ = 0;

    bool close_after_write_ = false;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(
        libp2p::connection::YamuxedConnection);
  };

}  // namespace libp2p::connection
