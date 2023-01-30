/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify/identify_delta.hpp>

#include <string>
#include <unordered_set>

#include <generated/protocol/identify/protobuf/identify.pb.h>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <libp2p/protocol/identify/utils.hpp>

namespace {
  const std::string kIdentifyDeltaProtocol = "/p2p/id/delta/1.0.0";
}  // namespace

namespace libp2p::protocol {
  IdentifyDelta::IdentifyDelta(Host &host,
                               network::ConnectionManager &conn_manager,
                               event::Bus &bus)
      : host_{host}, conn_manager_{conn_manager}, bus_{bus} {}

  peer::ProtocolName IdentifyDelta::getProtocolId() const {
    return kIdentifyDeltaProtocol;
  }

  void IdentifyDelta::handle(StreamAndProtocol stream) {
    // receive a Delta message
    auto rw = std::make_shared<basic::ProtobufMessageReadWriter>(stream.stream);
    rw->read<identify::pb::Identify>(
        [self{shared_from_this()},
         s = std::move(stream.stream)](auto &&msg_res) {
          self->deltaReceived(std::forward<decltype(msg_res)>(msg_res), s);
        });
  }

  void IdentifyDelta::start() {
    new_protos_sub_ =
        bus_.getChannel<event::network::ProtocolsAddedChannel>().subscribe(
            [self{weak_from_this()}](std::vector<peer::ProtocolName> new_protos) {
              if (auto s = self.lock()) {
                return self.lock()->sendDelta(
                    new_protos, gsl::span<const peer::ProtocolName>());
              }
            });
    rm_protos_sub_ =
        bus_.getChannel<event::network::ProtocolsRemovedChannel>().subscribe(
            [self{weak_from_this()}](std::vector<peer::ProtocolName> rm_protos) {
              if (auto s = self.lock()) {
                return self.lock()->sendDelta(gsl::span<const peer::ProtocolName>(),
                                              rm_protos);
              }
            });
  }

  void IdentifyDelta::deltaReceived(
      outcome::result<identify::pb::Identify> msg_res,
      const std::shared_ptr<connection::Stream> &stream) {
    auto [peer_id_str, peer_addr_str] = detail::getPeerIdentity(stream);
    if (!msg_res) {
      log_->error("cannot read identify-delta message from peer {}, {}: {}",
                  peer_id_str, peer_addr_str, msg_res.error().message());
      return stream->reset();
    }

    auto &&id_msg = std::move(msg_res.value());
    if (!id_msg.has_delta()) {
      log_->error(
          "peer initiated a stream with IdentifyDelta, but sent something "
          "else; peer {}, {}",
          peer_id_str, peer_addr_str);
      return stream->reset();
    }

    log_->info("received an IdentifyDelta message from peer {}, {}",
               peer_id_str, peer_addr_str);
    stream->close([self{shared_from_this()}, s = stream,
                   p = std::move(peer_id_str),
                   a = std::move(peer_addr_str)](auto &&res) {
      if (!res) {
        self->log_->error("cannot close stream to peer {}, {}: {}", p, a,
                          res.error().message());
      }
    });

    auto peer_id_res = stream->remotePeerId();
    if (!peer_id_res) {
      return;
      ;
      return log_->error("cannot retrieve peer id from the stream");
    }
    auto &&peer_id = peer_id_res.value();

    auto &&delta_msg = id_msg.delta();
    auto &proto_repo = host_.getPeerRepository().getProtocolRepository();

    // more beautiful ways cause compile errors :(
    std::vector<peer::ProtocolName> added_protocols;
    added_protocols.reserve(delta_msg.added_protocols().size());
    for (const auto &proto : delta_msg.added_protocols()) {
      added_protocols.push_back(proto);
    }
    auto add_res = proto_repo.addProtocols(peer_id, added_protocols);
    if (!add_res) {
      log_->error("cannot add new protocols of peer {}, {}: {}", peer_id_str,
                  peer_addr_str, add_res.error().message());
    }

    std::vector<peer::ProtocolName> rm_protocols;
    rm_protocols.reserve(delta_msg.rm_protocols().size());
    for (const auto &proto : delta_msg.rm_protocols()) {
      rm_protocols.push_back(proto);
    }
    auto rm_res = proto_repo.removeProtocols(peer_id, rm_protocols);
    if (!rm_res) {
      log_->error("cannot remove protocols of peer {}, {}: {}", peer_id_str,
                  peer_addr_str, rm_res.error().message());
    }
  }

  void IdentifyDelta::sendDelta(gsl::span<const peer::ProtocolName> added,
                                gsl::span<const peer::ProtocolName> removed) {
    auto msg = std::make_shared<identify::pb::Identify>();
    for (const auto &proto : added) {
      msg->mutable_delta()->add_added_protocols(proto);
    }
    for (const auto &proto : removed) {
      msg->mutable_delta()->add_rm_protocols(proto);
    }

    detail::streamToEachConnectedPeer(
        host_, conn_manager_, {kIdentifyDeltaProtocol},
        [self{shared_from_this()}, msg](auto s_res) {
          if (!s_res) {
            return;
          }
          self->sendDelta(std::move(s_res.value().stream), msg);
        });
  }

  void IdentifyDelta::sendDelta(
      std::shared_ptr<connection::Stream> stream,
      const std::shared_ptr<identify::pb::Identify> &msg) const {
    auto rw = std::make_shared<basic::ProtobufMessageReadWriter>(stream);
    rw->write<identify::pb::Identify>(
        *msg, [self{shared_from_this()}, s = std::move(stream)](auto &&res) {
          if (!res) {
            self->log_->error("cannot write Identify-Delta to peer {}, {}: {}",
                              s->remotePeerId().value().toBase58(),
                              s->remoteMultiaddr().value().getStringAddress(),
                              res.error().message());
          }
        });
  }
}  // namespace libp2p::protocol
