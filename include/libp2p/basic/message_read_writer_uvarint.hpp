/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MESSAGE_READ_WRITER_UVARINT_HPP
#define LIBP2P_MESSAGE_READ_WRITER_UVARINT_HPP

#include <memory>
#include <vector>

#include <gsl/span>
#include <libp2p/basic/message_read_writer.hpp>
#include <libp2p/basic/readwriter.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::basic {
  /**
   * Allows to read and write messages, which are prepended with a uvarint -
   * standard, for example, for Protobuf messages in Libp2p
   */
  class MessageReadWriterUvarint
      : public MessageReadWriter,
        public std::enable_shared_from_this<MessageReadWriterUvarint> {
   public:
    /**
     * Create an instance of MessageReadWriter
     * @param conn, from which to read/write messages
     */
    explicit MessageReadWriterUvarint(std::shared_ptr<ReadWriter> conn);

    /**
     * Read a message, which is prepended with a uvarint
     * @param cb, which is called, when the message is read or error happens
     */
    void read(ReadCallbackFunc cb) override;

    /**
     * Write a message; uvarint with its length will be prepended to it
     * @param buffer - the message to be written
     * @param cb, which is called, when the message is read or error happens
     */
    void write(gsl::span<const uint8_t> buffer,
               Writer::WriteCallbackFunc cb) override;

   private:
    std::shared_ptr<ReadWriter> conn_;
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_MESSAGE_READ_WRITER_UVARINT_HPP
