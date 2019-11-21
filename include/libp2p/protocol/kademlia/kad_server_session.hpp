/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_SERVER_SESSION_HPP
#define LIBP2P_KAD_SERVER_SESSION_HPP

#include <libp2p/protocol/kademlia/kad_message.hpp>
#include <memory>
#include <functional>

namespace libp2p::protocol::kademlia {

  class KadServerSession : public std::enable_shared_from_this<KadServerSession> {
   public:
    virtual ~KadServerSession() = default;

    virtual void start(std::shared_ptr<basic::ReadWriter> conn, MessageCallback cb) = 0;
    virtual void reply(const Message& msg) = 0;
    virtual void stop() = 0;
  };

  using KadServerSessionCreate = std::function<std::shared_ptr<KadServerSession>(/*args*/)>;

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_SERVER_SESSION_HPP
