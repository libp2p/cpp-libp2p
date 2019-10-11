/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MESSAGE_READER_HPP
#define LIBP2P_MESSAGE_READER_HPP

#include <memory>

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/basic/readwritecloser.hpp>
#include <libp2p/protocol_muxer/multiselect/connection_state.hpp>
#include <libp2p/protocol_muxer/multiselect/message_manager.hpp>

namespace libp2p::protocol_muxer {
  class Multiselect;

  /**
   * Reads messages of Multiselect format
   */
  class MessageReader {
   public:
    /**
     * Read next Multistream message
     * @param connection_state - state of the connection
     * @note will call Multiselect->onReadCompleted(..) after successful read
     */
    static void readNextMessage(
        std::shared_ptr<ConnectionState> connection_state);

   private:
    /**
     * Read next varint from the connection
     * @param connection_state - state of the connection
     */
    static void readNextVarint(
        std::shared_ptr<ConnectionState> connection_state);

    /**
     * Completion handler of varint read operation
     * @param connection_state - state of the connection
     */
    static void onReadVarintCompleted(
        std::shared_ptr<ConnectionState> connection_state);

    /**
     * Read specified number of bytes from the connection
     * @param connection_state - state of the connection
     * @param bytes_to_read - how much bytes are to be read
     * @param final_callback - in case of success, this callback is called
     */
    static void readNextBytes(
        std::shared_ptr<ConnectionState> connection_state,
        uint64_t bytes_to_read,
        std::function<void(std::shared_ptr<ConnectionState>)> final_callback);

    /**
     * Completion handler for read bytes operation in case a single line was
     * expected to be read
     * @param connection_state - state of the connection
     * @param read_bytes - how much bytes were read (or in this line)
     */
    static void onReadLineCompleted(
        const std::shared_ptr<ConnectionState> &connection_state,
        uint64_t read_bytes);
  };
}  // namespace libp2p::protocol_muxer

#endif  // LIBP2P_MESSAGE_READER_HPP
