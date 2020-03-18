/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arpa/inet.h>
#include <cstring>
#include <libp2p/common/byteutil.hpp>
#include <libp2p/muxer/yamux/yamux_frame.hpp>

#define TRACE_ENABLED 0
#include <libp2p/common/trace.hpp>

namespace libp2p::connection {
  YamuxFrame::ByteArray YamuxFrame::frameBytes(uint8_t version, FrameType type,
                                               Flag flag, uint32_t stream_id,
                                               uint32_t length,
                                               gsl::span<const uint8_t> data) {
    using common::putUint16BE;
    using common::putUint32BE;
    using common::putUint8;

    ByteArray bytes;
    bytes.reserve(kHeaderLength);  // minimum header size
    // TODO(akvinikym) 03.10.19 PRE-319: refine the functions
    putUint32BE(putUint32BE(putUint16BE(putUint8(putUint8(bytes, version),
                                                 static_cast<uint8_t>(type)),
                                        static_cast<uint16_t>(flag)),
                            stream_id),
                length);
    bytes.insert(bytes.end(), data.begin(), data.end());
    return bytes;
  }

  bool YamuxFrame::flagIsSet(Flag flag) const {
    return (static_cast<uint16_t>(flag) & flags) != 0;
  }

  YamuxFrame::ByteArray newStreamMsg(YamuxFrame::StreamId stream_id) {
    TRACE("yamux newStreamMsg, stream_id={}", stream_id);
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::SYN, stream_id, 0);
  }

  YamuxFrame::ByteArray ackStreamMsg(YamuxFrame::StreamId stream_id) {
    TRACE("yamux ackStreamMsg, stream_id={}", stream_id);
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::ACK, stream_id, 0);
  }

  YamuxFrame::ByteArray closeStreamMsg(YamuxFrame::StreamId stream_id) {
    TRACE("yamux closeStreamMsg, stream_id={}", stream_id);
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::FIN, stream_id, 0);
  }

  YamuxFrame::ByteArray resetStreamMsg(YamuxFrame::StreamId stream_id) {
    TRACE("yamux resetStreamMsg, stream_id={}", stream_id);
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::RST, stream_id, 0);
  }

  YamuxFrame::ByteArray pingOutMsg(uint32_t value) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::PING,
                                  YamuxFrame::Flag::SYN, 0, value);
  }

  YamuxFrame::ByteArray pingResponseMsg(uint32_t value) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::PING,
                                  YamuxFrame::Flag::ACK, 0, value);
  }

  YamuxFrame::ByteArray dataMsg(YamuxFrame::StreamId stream_id,
                                gsl::span<const uint8_t> data) {
    TRACE("yamux dataMsg, stream_id={}, size={}", stream_id, data.size());
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::NONE, stream_id,
                                  static_cast<uint32_t>(data.size()), data);
  }

  YamuxFrame::ByteArray goAwayMsg(YamuxFrame::GoAwayError error) {
    TRACE("yamux goAwayMsg");
    return YamuxFrame::frameBytes(
        YamuxFrame::kDefaultVersion, YamuxFrame::FrameType::GO_AWAY,
        YamuxFrame::Flag::NONE, 0, static_cast<uint32_t>(error));
  }

  YamuxFrame::ByteArray windowUpdateMsg(YamuxFrame::StreamId stream_id,
                                        uint32_t window_delta) {
    return YamuxFrame::frameBytes(
        YamuxFrame::kDefaultVersion, YamuxFrame::FrameType::WINDOW_UPDATE,
        YamuxFrame::Flag::NONE, stream_id, window_delta);
  }

  boost::optional<YamuxFrame> parseFrame(gsl::span<const uint8_t> frame_bytes) {
    if (frame_bytes.size() < static_cast<int>(YamuxFrame::kHeaderLength)) {
      return {};
    }

    YamuxFrame frame{};

    frame.version = frame_bytes[0];

    switch (frame_bytes[1]) {
      case 0:
        frame.type = YamuxFrame::FrameType::DATA;
        break;
      case 1:
        frame.type = YamuxFrame::FrameType::WINDOW_UPDATE;
        break;
      case 2:
        frame.type = YamuxFrame::FrameType::PING;
        break;
      case 3:
        frame.type = YamuxFrame::FrameType::GO_AWAY;
        break;
      default:
        return {};
    }

    // NOLINTNEXTLINE
    frame.flags = ntohs(common::convert<uint16_t>(&frame_bytes[2]));
    // NOLINTNEXTLINE
    frame.stream_id = ntohl(common::convert<uint32_t>(&frame_bytes[4]));
    // NOLINTNEXTLINE
    frame.length = ntohl(common::convert<uint32_t>(&frame_bytes[8]));

    const auto &data_begin = frame_bytes.begin() + YamuxFrame::kHeaderLength;
    if (data_begin != frame_bytes.end()) {
      frame.data = std::vector<uint8_t>(data_begin, frame_bytes.end());
    }

    return frame;
  }
}  // namespace libp2p::connection
