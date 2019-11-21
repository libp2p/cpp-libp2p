/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_MESSAGE_READ_WRITER_HPP
#define LIBP2P_KAD_MESSAGE_READ_WRITER_HPP

#include <libp2p/basic/message_read_writer.hpp>
#include "kad_message.hpp"
#include "kad2_common.hpp"

namespace libp2p::kad2 {

 class MessageReadWriter {
  public:
   using ReadResultFn = std::function<void(outcome::result<Message>)>;
   using WriteResultFn = basic::Writer::WriteCallbackFunc;

   MessageReadWriter(
     std::shared_ptr<basic::ReadWriter> conn,
     ReadResultFn rr, WriteResultFn wr
   );

   ~MessageReadWriter() = default;

   void read();

   void write(const Message& msg);

  private:
   void onRead(basic::MessageReadWriter::ReadCallback result);

   libp2p::common::Logger log_;
   basic::MessageReadWriter& mrw_;
   std::vector<uint8_t> buffer_;
   ReadResultFn read_cb_;
   WriteResultFn write_cb_;
 };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_MESSAGE_READ_WRITER_HPP
