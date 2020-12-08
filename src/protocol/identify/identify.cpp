/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify/identify.hpp>

#include <string>
#include <tuple>

#include <boost/assert.hpp>

namespace {
  const std::string kIdentifyProto = "/ipfs/id/1.0.0";
}  // namespace

namespace libp2p::protocol {
  Identify::Identify(Host &host,
                     std::shared_ptr<IdentifyMessageProcessor> msg_processor,
                     event::Bus &event_bus)
      : host_{host}, msg_processor_{std::move(msg_processor)}, bus_{event_bus} {
    BOOST_ASSERT(msg_processor_);
  }

  boost::signals2::connection Identify::onIdentifyReceived(
      const std::function<IdentifyMessageProcessor::IdentifyCallback> &cb) {
    return msg_processor_->onIdentifyReceived(cb);
  }

  std::vector<multi::Multiaddress> Identify::getAllObservedAddresses() const {
    return msg_processor_->getObservedAddresses().getAllAddresses();
  }

  std::vector<multi::Multiaddress> Identify::getObservedAddressesFor(
      const multi::Multiaddress &address) const {
    return msg_processor_->getObservedAddresses().getAddressesFor(address);
  }

  peer::Protocol Identify::getProtocolId() const {
    return kIdentifyProto;
  }

  void Identify::handle(StreamResult stream_res) {
    if (!stream_res) {
      return;
    }
    msg_processor_->sendIdentify(std::move(stream_res.value()));
  }

  void Identify::start() {
    // no double starts
    BOOST_ASSERT(!started_);
    started_ = true;

    host_.setProtocolHandler(
        kIdentifyProto,
        [wp = weak_from_this()](protocol::BaseProtocol::StreamResult rstream) {
          if (auto self = wp.lock()) {
            self->handle(std::move(rstream));
          }
        });

    sub_ = bus_.getChannel<network::event::OnNewConnectionChannel>().subscribe(
        [wp = weak_from_this()](auto &&conn) {
          if (auto self = wp.lock()) {
            return self->onNewConnection(conn);
          }
        });
  }

  void Identify::onNewConnection(
      const std::weak_ptr<connection::CapableConnection> &conn) {
    if (conn.expired()) {
      return;
    }

    auto remote_peer_res = conn.lock()->remotePeer();
    if (!remote_peer_res) {
      return;
    }

    auto remote_peer_addr_res = conn.lock()->remoteMultiaddr();
    if (!remote_peer_addr_res) {
      return;
    }

    peer::PeerInfo peer_info{std::move(remote_peer_res.value()),
                             std::vector<multi::Multiaddress>{
                                 std::move(remote_peer_addr_res.value())}};

    msg_processor_->getHost().newStream(
        peer_info, kIdentifyProto,
        [self{shared_from_this()}](auto &&stream_res) {
          if (!stream_res) {
            return;
          }
          self->msg_processor_->receiveIdentify(std::move(stream_res.value()));
        });
  }
}  // namespace libp2p::protocol
