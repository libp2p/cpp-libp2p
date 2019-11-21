/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_SERVER_SESSION_IMPL_HPP
#define LIBP2P_KAD_SERVER_SESSION_IMPL_HPP

#include <libp2p/protocol/kademlia/kad_server_session.hpp>
#include <libp2p/protocol/kademlia/message_read_writer.hpp>


namespace libp2p::protocol::kademlia {

  class KadServerSessionImpl : public KadServerSession {
   public:
    static std::shared_ptr<KadServerSession> create(/*args*/);

    ~KadServerSessionImpl() override = default;



    void start(MessageCallback cb) override;

    void reply(const Message& msg);

    void stop();

   private:
    KadServerSessionImpl(/* */);


    //std::shared_ptr<connection::Stream> stream_;
    MessageReadWriter mrw_;
    //common::Logger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_SERVER_SESSION_IMPL_HPP
