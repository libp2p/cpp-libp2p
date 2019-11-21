/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_CLIENT_SESSION_HPP
#define LIBP2P_KAD_CLIENT_SESSION_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/protocol/kademlia/message_read_writer.hpp>

namespace libp2p::protocol::kademlia {

  class KadClientSession {
   public:
    KadClientSession(
      libp2p::common::Logger log, std::shared_ptr<connection::Stream> stream,
      MessageReadWriter::ReadResultFn rr, MessageReadWriter::WriteResultFn wr
    );

    void send(const Message& msg);

    void close();

   private:
    std::shared_ptr<connection::Stream> stream_;
    MessageReadWriter mrw_;
  };

}  // namespace libp2p::protocol::kademlia

#endif //LIBP2P_KAD_CLIENT_SESSION_HPP
