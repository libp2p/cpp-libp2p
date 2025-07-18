/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include <libp2p/basic/read_buffer.hpp>
#include <libp2p/basic/write_queue.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/stream.hpp>

namespace libp2p::connection {

  class YamuxedConnection;

  /// Yamux specific feedback interface, stream->connection
  class YamuxStreamFeedback {
   public:
    virtual ~YamuxStreamFeedback() = default;

    /// Stream transfers data to connection
    virtual void writeStreamData(uint32_t stream_id, BytesIn data) = 0;

    /// Stream acknowledges received bytes
    virtual void ackReceivedBytes(uint32_t stream_id, uint32_t bytes) = 0;

    /// Stream defers callback to avoid reentrancy
    virtual void deferCall(std::function<void()>) = 0;

    /// Stream closes
    virtual void resetStream(uint32_t stream_id) = 0;

    /// Stream closed, remove from active streams if 2FINs were sent
    virtual void streamClosed(uint32_t stream_id) = 0;
  };

  /// Stream implementation, used by Yamux multiplexer
  class YamuxStream final : public Stream,
                            public std::enable_shared_from_this<YamuxStream> {
   public:
    YamuxStream(const YamuxStream &other) = delete;
    YamuxStream &operator=(const YamuxStream &other) = delete;
    YamuxStream(YamuxStream &&other) = delete;
    YamuxStream &operator=(YamuxStream &&other) = delete;
    ~YamuxStream() override = default;

    YamuxStream(std::shared_ptr<connection::SecureConnection> connection,
                YamuxStreamFeedback &feedback,
                uint32_t stream_id,
                size_t maximum_window_size,
                size_t write_queue_limit);

    void readSome(BytesOut out, ReadCallbackFunc cb) override;

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    void writeSome(BytesIn in, WriteCallbackFunc cb) override;

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

    bool isClosed() const override;

    void close(VoidResultHandlerFunc cb) override;

    bool isClosedForRead() const override;

    bool isClosedForWrite() const override;

    void reset() override;

    void adjustWindowSize(uint32_t new_size, VoidResultHandlerFunc cb) override;

    outcome::result<peer::PeerId> remotePeerId() const override;

    outcome::result<bool> isInitiator() const override;

    outcome::result<multi::Multiaddress> localMultiaddr() const override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() const override;

    /// Increases send window. Called from Connection
    void increaseSendWindow(size_t delta);

    enum DataFromConnectionResult {
      kKeepStream,
      kRemoveStream,
      kRemoveStreamAndSendRst,
    };

    /// Called from Connection. New data received
    /// Returns kRemoveStreamAndSendRst on window overflow
    DataFromConnectionResult onDataReceived(BytesOut bytes);

    /// Called from Connection on FIN received
    /// Returns kRemoveStream if FIN was sent from this side
    DataFromConnectionResult onFINReceived();

    /// Called from Connection, stream was reset by peer
    void onRSTReceived();

    /// Data written into the wire. Called from Connection
    void onDataWritten(size_t bytes);

    /// Connection closed by network error
    void closedByConnection(std::error_code ec);

   private:
    struct Reading {
      BytesOut out;
      ReadCallbackFunc cb;
    };

    /// Performs close-related cleanup and notifications
    void doClose(std::error_code ec);

    /// Called by read*() functions
    void doRead(BytesOut out, ReadCallbackFunc cb);

    /// Dequeues data from write queue and sends to the wire in async manner
    void doWrite();

    /// Called by write*() functions
    void doWrite(BytesIn in, WriteCallbackFunc cb);

    /// Clears close callback state
    [[nodiscard]] std::pair<VoidResultHandlerFunc, outcome::result<void>>
    closeCompleted();

    /// Underlying connection (secured)
    std::shared_ptr<connection::SecureConnection> connection_;

    /// Yamux-specific interface of connection
    YamuxStreamFeedback &feedback_;

    /// Stream ID
    uint32_t stream_id_;

    /// True if the stream is readable, until FIN received
    bool is_readable_ = true;

    /// True if the stream is writable, until FIN sent
    bool is_writable_ = true;

    /// True after FIN sent
    bool fin_sent_ = false;

    /// Non zero reason means that stream is closed and the reason of it
    std::optional<std::error_code> close_reason_;

    /// Max bytes allowed to send
    size_t window_size_;

    /// Receive window size: max buffered unreceived bytes
    size_t peers_window_size_;

    /// Maximum window size allowed for peer
    size_t maximum_window_size_;

    /// Write queue with callbacks
    basic::WriteQueue write_queue_;

    /// Internal read buffer, stores bytes received between read()s
    basic::ReadBuffer internal_read_buffer_;

    /// True if read operation is active
    std::optional<Reading> reading_;

    /// adjustWindowSize() callback, triggers when receive window size
    /// becomes greater or equal then desired
    VoidResultHandlerFunc window_size_cb_;

    /// Close callback
    VoidResultHandlerFunc close_cb_;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::connection::YamuxStream);
  };

}  // namespace libp2p::connection
