/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_WEBSOCKET_WSREADINGSTATE
#define LIBP2P_CONNECTION_WEBSOCKET_WSREADINGSTATE

#include <libp2p/basic/read_buffer.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::connection::websocket {

  /// Buffered reader and segmenter for ws inbound data
  class WsReadingState {
   public:
    /// Callback on headers, returns false to terminate further processing
    using HeaderCallback =
        std::function<bool(boost::optional<bool> header)>;

    /// Callback on data segments
    using DataCallback = std::function<void(
        gsl::span<uint8_t> segment, bool fin)>;

    WsReadingState(HeaderCallback on_header, DataCallback on_data);

    /// Data received from wire, collect it and segment into frames.
    /// NOTE: cuts bytes from the head of bytes_read
    void onDataReceived(gsl::span<uint8_t> &bytes_read);

    /// Discards data for current message being read.
    /// Reentrant function, called from callbacks
    void discardDataMessage();

    /// Resets everything to reading header state
    void reset();

   private:
    /// Processes header segmented from incoming data stream
    bool processHeader(gsl::span<uint8_t> &bytes_read);

    /// Processes data message fragment from incoming data stream
    void processData(gsl::span<uint8_t> &bytes_read);

    /// Header cb
    HeaderCallback on_header_;

    /// Data cb
    DataCallback on_data_;

    /// Header being read from incoming bytes
    basic::FixedBufferCollector header_;

    /// Message bytes not yet read from incoming data
    size_t data_bytes_unread_ = 0;

    /// Send FIN flag to stream with final data fragment
    bool fin_after_data_ = false;
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_CONNECTION_WEBSOCKET_WSREADINGSTATE
