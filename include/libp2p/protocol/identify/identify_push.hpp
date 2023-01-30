/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_IDENTIFY_PUSH_HPP
#define LIBP2P_IDENTIFY_PUSH_HPP

#include <memory>
#include <vector>

#include <libp2p/event/bus.hpp>
#include <libp2p/protocol/base_protocol.hpp>
#include <libp2p/protocol/identify/identify_msg_processor.hpp>

namespace libp2p::protocol {
  /**
   * Implementation of Identify-Push protocol, which is used to inform known the
   * peers about changes in this peer's configuration by sending or receiving a
   * whole Identify message. Read more:
   * https://github.com/libp2p/specs/blob/master/identify/README.md
   */
  class IdentifyPush : public BaseProtocol,
                       public std::enable_shared_from_this<IdentifyPush> {
   public:
    IdentifyPush(std::shared_ptr<IdentifyMessageProcessor> msg_processor,
                 event::Bus &bus);

    peer::ProtocolName getProtocolId() const override;

    /**
     * In Identify-Push, handle means we accepted an Identify-Push stream and
     * should receive an Identify message
     */
    void handle(StreamAndProtocol stream) override;

    /**
     * Start this Identify-Push, so that it subscribes to events, when some
     * basic info of our peer changes
     */
    void start();

   private:
    /**
     * Send an Identify message to all peers we are connected to
     */
    void sendPush();

    std::shared_ptr<IdentifyMessageProcessor> msg_processor_;

    event::Bus &bus_;
    std::vector<event::Handle> sub_handles_;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_IDENTIFY_PUSH_HPP
