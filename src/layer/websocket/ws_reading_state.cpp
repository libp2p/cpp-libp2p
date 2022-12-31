/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_reading_state.hpp>

#include <cassert>

#include <libp2p/log/logger.hpp>

namespace libp2p::connection::websocket {

  namespace {
    auto log() {
      static auto logger = log::createLogger("WsConn");
      return logger.get();
    }

    inline size_t size(const gsl::span<uint8_t> &span) {
      return static_cast<size_t>(span.size());
    }

    inline std::tuple<gsl::span<uint8_t>, gsl::span<uint8_t>> split(
        gsl::span<uint8_t> span, size_t n) {
      return {span.subspan(0, ssize_t(n)), span.subspan(ssize_t(n))};
    }

  }  // namespace

  WsReadingState::WsReadingState(HeaderCallback on_header, DataCallback on_data)
      : on_header_(std::move(on_header)),
        on_data_(std::move(on_data)),
        header_(WsFrame::kHeaderLength) {
    assert(on_header_);
    assert(on_data_);
  }

  void WsReadingState::onDataReceived(gsl::span<uint8_t> &bytes_read) {
    bool proceed = true;
    while (!bytes_read.empty() && proceed) {
      if (data_bytes_unread_ == 0) {
        proceed = processHeader(bytes_read);
      } else {
        processData(bytes_read);
      }
    }
  }

  void WsReadingState::processData(gsl::span<uint8_t> &bytes_read) {
    assert(data_bytes_unread_ > 0);

    auto bytes_available = size(bytes_read);

    auto n = data_bytes_unread_;
    if (n > bytes_available) {
      // data message is partial, will be consumed inside stream or
      // discarded
      n = bytes_available;
    }
    auto [head, tail] = split(bytes_read, n);
    data_bytes_unread_ -= n;
    bytes_read = tail;

    bool fin = false;

    if (data_bytes_unread_ == 0) {
      fin = fin_after_data_;
      reset();
    }

    on_data_(head, fin);
  }

  bool WsReadingState::processHeader(gsl::span<uint8_t> &bytes_read) {
    assert(data_bytes_unread_ == 0);

    auto maybe_header = header_.add(bytes_read);
    if (!maybe_header) {
      // more data needed
      return false;
    }

    auto maybe_frame = parseFrame(maybe_header.value());
    if (maybe_frame.has_value()) {
      auto &frame = maybe_frame.value();

      bool non_zero_data =
          (frame.type == WsFrame::FrameType::DATA && frame.length > 0);

      if (non_zero_data) {
        data_bytes_unread_ = frame.length;

        // // these flags arrive with final data fragment
        // if (frame.stream_id != 0) {
        //   fin_after_data_ = frame.flagIsSet(WsFrame::Flag::FIN);
        //
        //   frame.flags &= ~(static_cast<uint16_t>(WsFrame::Flag::RST)
        //                    | static_cast<uint16_t>(WsFrame::Flag::FIN));
        // }
      }

      header_.expect(WsFrame::kHeaderLength);
    }
    return on_header_(std::move(maybe_frame));
  }

  void WsReadingState::discardDataMessage() {
    fin_after_data_ = false;
  }

  void WsReadingState::reset() {
    header_.expect(WsFrame::kHeaderLength);
    data_bytes_unread_ = 0;
    discardDataMessage();
  }

}  // namespace libp2p::connection::websocket
