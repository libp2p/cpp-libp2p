/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/message_read_writer.hpp>
#include <libp2p/basic/readwriter.hpp>

namespace libp2p::basic {
  /**
   * Allows to read and write messages, which are prepended with a message
   * length as 32-byte number passed in network byte order (Big Endian). For
   * example, this is used in SECIO security protocol.
   */
  class MessageReadWriterBigEndian
      : public MessageReadWriter,
        public std::enable_shared_from_this<MessageReadWriterBigEndian> {
   public:
    static constexpr auto kLenMarkerSize = sizeof(uint32_t);

    /**
     * Create an instance of MessageReadWriter
     * @param conn, from which to read/write messages
     */
    explicit MessageReadWriterBigEndian(std::shared_ptr<ReadWriter> conn);

    /**
     * Read a message, which is prepended with a 32-byte Big Endian lenght
     * @param cb, which is called, when the message is read or error happens
     */
    void read(ReadCallbackFunc cb) override;

    /**
     * Write a message; 32-byte Big Endian number with its length will be
     * prepended to it
     * @param buffer - the message to be written
     * @param cb, which is called, when the message is read or error happens
     */
    void write(BytesIn buffer, CbOutcomeVoid cb) override;

   private:
    std::shared_ptr<ReadWriter> conn_;
  };
}  // namespace libp2p::basic
