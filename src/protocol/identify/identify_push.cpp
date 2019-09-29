/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/identify_push.hpp"

#include <string>

#include <libp2p/network/listener_manager.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/protocol/identify/utils.hpp>

namespace {
  const std::string kIdentifyPushProtocol = "/ipfs/id/push/1.0.0";
}

namespace libp2p::protocol {
  IdentifyPush::IdentifyPush(
      std::shared_ptr<IdentifyMessageProcessor> msg_processor, event::Bus &bus)
      : msg_processor_{std::move(msg_processor)}, bus_{bus} {}

  peer::Protocol IdentifyPush::getProtocolId() const {
    return kIdentifyPushProtocol;
  }

  void IdentifyPush::handle(StreamResult stream_res) {
    if (!stream_res) {
      return;
    }
    msg_processor_->receiveIdentify(std::move(stream_res.value()));
  }

  void IdentifyPush::start() {
    static constexpr uint8_t kChannelsAmount = 3;

    auto send_push = [self{weak_from_this()}](auto && /*ignored*/) {
      if (self.expired()) {
        return;
      }
      self.lock()->sendPush();
    };

    sub_handles_.reserve(kChannelsAmount);
    sub_handles_.push_back(
        bus_.getChannel<network::event::ListenAddressAddedChannel>().subscribe(
            send_push));
    sub_handles_.push_back(
        bus_.getChannel<network::event::ListenAddressRemovedChannel>()
            .subscribe(send_push));
    sub_handles_.push_back(
        bus_.getChannel<peer::event::KeyPairChangedChannel>().subscribe(
            std::move(send_push)));
  }

  void IdentifyPush::sendPush() {
    detail::streamToEachConnectedPeer(
        msg_processor_->getHost(), msg_processor_->getConnectionManager(),
        kIdentifyPushProtocol, [self{weak_from_this()}](auto &&s_res) {
          if (!s_res) {
            return;
          }
          if (auto t = self.lock()) {
            return t->msg_processor_->sendIdentify(std::move(s_res.value()));
          }
        });
  }
}  // namespace libp2p::protocol
