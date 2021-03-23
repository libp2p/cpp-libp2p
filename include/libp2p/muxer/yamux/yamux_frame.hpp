/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_YAMUX_FRAME_HPP
#define LIBP2P_YAMUX_FRAME_HPP

#include <boost/optional.hpp>
#include <gsl/span>

#include <libp2p/common/types.hpp>

namespace libp2p::connection {
  /**
   * Header which is sent and accepted with Yamux protocol
   */
  struct YamuxFrame {
    using ByteArray = common::ByteArray;
    using StreamId = uint32_t;
    static constexpr uint32_t kHeaderLength = 12;

    enum class FrameType : uint8_t {
      DATA = 0,           // transmit data
      WINDOW_UPDATE = 1,  // update the sender's receive window size
      PING = 2,           // ping for various purposes
      GO_AWAY = 3         // close the session
    };
    enum class Flag : uint16_t {
      NONE = 0,  // no flag is set
      SYN = 1,   // start of a new stream
      ACK = 2,   // acknowledge start of a new stream
      FIN = 4,   // half-close of the stream
      RST = 8    // reset a stream
    };
    enum class GoAwayError : uint32_t {
      NORMAL = 0,
      PROTOCOL_ERROR = 1,
      INTERNAL_ERROR = 2
    };
    static constexpr uint8_t kDefaultVersion = 0;
    static constexpr uint32_t kInitialWindowSize = 256 * 1024;

    uint8_t version;
    FrameType type;
    uint16_t flags;
    StreamId stream_id;
    uint32_t length;

    /**
     * Get bytes representation of the Yamux frame with given parameters
     * @return bytes of the frame
     *
     * @note even though Flag should be a number, in our implementation we do
     * not send messages with more than one flag set, so enum can be accepted as
     * well
     */
    static ByteArray frameBytes(
        uint8_t version, FrameType type, Flag flag, uint32_t stream_id,
        uint32_t length, bool reserve_space = true);

    /**
     * Check if the (\param flag) is set in this frame
     * @return true, if the flag is set, false otherwise
     */
    bool flagIsSet(Flag flag) const;
  };

  /**
   * Create a message, which notifies about a new stream creation
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  common::ByteArray newStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message, which acknowledges a new stream creation
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  common::ByteArray ackStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message, which closes a stream for writes
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  common::ByteArray closeStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message, which resets a stream
   * @param stream_id to be put into the message
   * @return bytes of the message
   */
  common::ByteArray resetStreamMsg(YamuxFrame::StreamId stream_id);

  /**
   * Create a message with an outcoming ping
   * @param value - ping value to be put into the message
   * @return bytes of the message
   */
  common::ByteArray pingOutMsg(uint32_t value);

  /**
   * Create a message, which responses to a ping
   * @param value - ping value to be put into the message
   * @return bytes of the message
   */
  common::ByteArray pingResponseMsg(uint32_t value);

  /**
   * Create a message with some data inside
   * @param stream_id to be put into the message
   * @param data_length length field
   * @param reserve_space whether to allocate space for message
   * @return bytes of the message
   */
  common::ByteArray dataMsg(YamuxFrame::StreamId stream_id,
                            uint32_t data_length, bool reserve_space = true);

  /**
   * Create a message, which breaks a connection with a peer
   * @param error to be put into the message
   * @return bytes of the message
   */
  common::ByteArray goAwayMsg(YamuxFrame::GoAwayError error);

  /**
   * Create a window update message
   * @param stream_id to be put into the message
   * @param window_delta to be put into the message
   * @return bytes of the message
   */
  common::ByteArray windowUpdateMsg(YamuxFrame::StreamId stream_id,
                                    uint32_t window_delta);

  /**
   * Convert bytes into a frame object, if it is correct
   * @param frame_bytes to be converted
   * @return frame object, if convertation is successful, none otherwise
   */
  boost::optional<YamuxFrame> parseFrame(gsl::span<const uint8_t> frame_bytes);
}  // namespace libp2p::connection

#endif  // LIBP2P_YAMUX_FRAME_HPP
