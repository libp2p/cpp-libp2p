/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOBUF_MESSAGE_READ_WRITER_HPP
#define LIBP2P_PROTOBUF_MESSAGE_READ_WRITER_HPP

#include <libp2p/basic/message_read_writer.hpp>

namespace libp2p::basic {
  /**
   * Reader/writer for Protobuf messages; user of this class MUST ENSURE that no
   * two parallel reads or writes happens - this will lead to UB
   */
  class ProtobufMessageReadWriter
      : public std::enable_shared_from_this<ProtobufMessageReadWriter> {
    template <typename ProtoMsgType>
    using ReadCallbackFunc = std::function<void(outcome::result<ProtoMsgType>)>;

   public:
    /**
     * Create an instance of read/writer
     * @param read_writer, with the help of which messages are read & written
     */
    explicit ProtobufMessageReadWriter(
        std::shared_ptr<MessageReadWriter> read_writer);
    explicit ProtobufMessageReadWriter(std::shared_ptr<ReadWriter> conn);

    /**
     * Read a message from the connection
     * @tparam ProtoMsgType - type of the message to be read
     * @param cb to be called, when the message is read, or error happens
     * @param bytes vector under this shared_ptr if not nullptr will be filled
     * with raw bytes read *prior* to the moment cb execution begins
     */
    template <typename ProtoMsgType,
              typename = std::enable_if_t<
                  std::is_default_constructible<ProtoMsgType>::value>>
    void read(ReadCallbackFunc<ProtoMsgType> cb,
              const std::shared_ptr<std::vector<uint8_t>> &bytes = nullptr) {
      read_writer_->read(
          [self{shared_from_this()}, cb = std::move(cb), bytes](auto &&res) {
            if (!res) {
              return cb(res.error());
            }

            auto &&buf = res.value();
            ProtoMsgType msg;
            msg.ParseFromArray(buf->data(), buf->size());
            if (bytes) {
              bytes->clear();
              bytes->swap(*buf);
            }
            cb(std::move(msg));
          });
    }

    /**
     * Write a message to the connection
     * @tparam ProtoMsgType - type of the message to be written
     * @param msg to be written
     * @param cb to be called, when the message is written, or error happens
     * @param bytes vector under this shared_ptr if not nullptr will be filled
     * with raw bytes written *prior* to the moment of cb execution begins
     */
    template <typename ProtoMsgType,
              typename = std::enable_if_t<
                  std::is_default_constructible<ProtoMsgType>::value>>
    void write(const ProtoMsgType &msg, Writer::WriteCallbackFunc cb,
               const std::shared_ptr<std::vector<uint8_t>> &bytes = nullptr) {
      auto msg_bytes =
          std::make_shared<std::vector<uint8_t>>(msg.ByteSize(), 0);
      msg.SerializeToArray(msg_bytes->data(), msg.ByteSize());
      if (bytes) {
        bytes->clear();
        std::copy(msg_bytes->begin(), msg_bytes->end(),
                  std::back_inserter(*bytes));
      }
      read_writer_->write(*msg_bytes,
                          [cb = std::move(cb), msg_bytes](auto &&res) {
                            cb(std::forward<decltype(res)>(res));
                          });
    }

   private:
    std::shared_ptr<MessageReadWriter> read_writer_;
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_PROTOBUF_MESSAGE_READ_WRITER_HPP
