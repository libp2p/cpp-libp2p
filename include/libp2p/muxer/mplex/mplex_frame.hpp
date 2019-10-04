/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MPLEX_FRAME_HPP
#define LIBP2P_MPLEX_FRAME_HPP

#include <cstdint>
#include <functional>
#include <memory>

#include <boost/optional.hpp>
#include <libp2p/basic/readwriter.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/muxer/mplex/mplex_stream.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::connection {
  /**
   * Message, which is passed over the Mplex protocol
   */
  struct MplexFrame {
    enum class Flag : uint8_t {
      NEW_STREAM = 0,
      MESSAGE_RECEIVER = 1,
      MESSAGE_INITIATOR = 2,
      CLOSE_RECEIVER = 3,
      CLOSE_INITIATOR = 4,
      RESET_RECEIVER = 5,
      RESET_INITIATOR = 6
    };
    using Length = uint64_t;

    Flag flag;
    MplexStream::StreamNumber stream_number;
    Length length;
    common::ByteArray data;

    /**
     * Convert this frame to bytes
     * @return bytes representation of the frame
     */
    common::ByteArray toBytes() const;
  };

  /**
   * Create an MplexFrame and return its bytes representation
   * @return bytes of the MplexFrame
   */
  common::ByteArray createFrameBytes(MplexFrame::Flag flag,
                                     MplexStream::StreamNumber stream_number,
                                     common::ByteArray data = {});

  /**
   * Create an MplexFrame
   * @param id_flag - stream_id and flag, joined in a specific way, came from
   * the network
   * @param data to be in this frame
   * @return created frame or error
   */
  outcome::result<MplexFrame> createFrame(uint64_t id_flag,
                                          common::ByteArray data);

  /**
   * Read and parse MplexFrame bytes
   * @param reader, from which to read the bytes
   * @param cb, which is called, when bytes are and parsed, or error happens
   */
  void readFrame(std::shared_ptr<basic::ReadWriter> reader,
                 std::function<void(outcome::result<MplexFrame>)> cb);
}  // namespace libp2p::connection

#endif  // LIBP2P_MPLEX_FRAME_HPP
