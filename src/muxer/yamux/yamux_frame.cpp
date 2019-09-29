/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux_frame.hpp>

namespace {
  using ByteArray = libp2p::common::ByteArray;

  ByteArray &putUint8(ByteArray &buffer, uint8_t number) {
    buffer.push_back(number);
    return buffer;
  }

  ByteArray &putUint16NetworkOrder(ByteArray &buffer, uint16_t number) {
    buffer.push_back(static_cast<unsigned char &&>((number)&0xFFu));
    buffer.push_back(static_cast<unsigned char &&>((number >> 8u) & 0xFFu));
    return buffer;
  }

  ByteArray &putUint32NetworkOrder(ByteArray &buffer, uint32_t number) {
    buffer.push_back(static_cast<unsigned char &&>((number)&0xFFu));
    buffer.push_back(static_cast<unsigned char &&>((number >> 8u) & 0xFFu));
    buffer.push_back(static_cast<unsigned char &&>((number >> 16u) & 0xFFu));
    buffer.push_back(static_cast<unsigned char &&>((number >> 24u) & 0xFFu));
    return buffer;
  }
}  // namespace

namespace libp2p::connection {
  ByteArray YamuxFrame::frameBytes(uint8_t version, FrameType type, Flag flag,
                                   uint32_t stream_id, uint32_t length,
                                   gsl::span<const uint8_t> data) {
    // TODO(akvinikym) PRE-118 15.04.19: refactor with NetworkOrderEncoder, when
    // implemented
    ByteArray buffer{};
    putUint32NetworkOrder(
        putUint32NetworkOrder(
            putUint16NetworkOrder(
                putUint8(putUint8(buffer, version), static_cast<uint8_t>(type)),
                static_cast<uint16_t>(flag)),
            stream_id),
        length);
    // put data to the end
    buffer.insert(buffer.end(), data.begin(), data.end());
    return buffer;
  }

  ByteArray newStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::SYN, stream_id, 0);
  }

  ByteArray ackStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::ACK, stream_id, 0);
  }

  ByteArray closeStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::FIN, stream_id, 0);
  }

  ByteArray resetStreamMsg(YamuxFrame::StreamId stream_id) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::RST, stream_id, 0);
  }

  ByteArray pingOutMsg(uint32_t value) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::PING,
                                  YamuxFrame::Flag::SYN, 0, value);
  }

  ByteArray pingResponseMsg(uint32_t value) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::PING,
                                  YamuxFrame::Flag::ACK, 0, value);
  }

  ByteArray dataMsg(YamuxFrame::StreamId stream_id,
                    gsl::span<const uint8_t> data) {
    return YamuxFrame::frameBytes(YamuxFrame::kDefaultVersion,
                                  YamuxFrame::FrameType::DATA,
                                  YamuxFrame::Flag::SYN, stream_id,
                                  static_cast<uint32_t>(data.size()), data);
  }

  ByteArray goAwayMsg(YamuxFrame::GoAwayError error) {
    return YamuxFrame::frameBytes(
        YamuxFrame::kDefaultVersion, YamuxFrame::FrameType::GO_AWAY,
        YamuxFrame::Flag::SYN, 0, static_cast<uint32_t>(error));
  }

  ByteArray windowUpdateMsg(YamuxFrame::StreamId stream_id,
                            uint32_t window_delta) {
    return YamuxFrame::frameBytes(
        YamuxFrame::kDefaultVersion, YamuxFrame::FrameType::WINDOW_UPDATE,
        YamuxFrame::Flag::SYN, stream_id, window_delta);
  }

  boost::optional<YamuxFrame> parseFrame(gsl::span<const uint8_t> frame_bytes) {
    if (frame_bytes.size() < YamuxFrame::kHeaderLength) {
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

    // TODO(akvinikym) PRE-118 15.04.19: refactor with NetworkOrderEncoder, when
    // implemented

    switch ((static_cast<uint16_t>(frame_bytes[3]) << 8u) | frame_bytes[2]) {
      case 1:
        frame.flag = YamuxFrame::Flag::SYN;
        break;
      case 2:
        frame.flag = YamuxFrame::Flag::ACK;
        break;
      case 4:
        frame.flag = YamuxFrame::Flag::FIN;
        break;
      case 8:
        frame.flag = YamuxFrame::Flag::RST;
        break;
      default:
        return {};
    }

    frame.stream_id = (static_cast<uint32_t>(frame_bytes[7]) << 24u)
        | (static_cast<uint16_t>(frame_bytes[6]) << 16u)
        | (static_cast<uint16_t>(frame_bytes[5]) << 8u)
        | (static_cast<uint16_t>(frame_bytes[4]));

    frame.length = (static_cast<uint32_t>(frame_bytes[11]) << 24u)
        | (static_cast<uint16_t>(frame_bytes[10]) << 16u)
        | (static_cast<uint16_t>(frame_bytes[9]) << 8u)
        | (static_cast<uint16_t>(frame_bytes[8]));

    const auto &data_begin = frame_bytes.begin() + YamuxFrame::kHeaderLength;
    if (data_begin != frame_bytes.end()) {
      frame.data = std::vector<uint8_t>(data_begin, frame_bytes.end());
    };

    return frame;
  }
}  // namespace libp2p::connection
