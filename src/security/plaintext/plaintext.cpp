/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/plaintext/plaintext.hpp>

#include <functional>

#include <generated/security/plaintext/protobuf/plaintext.pb.h>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/error.hpp>
#include <libp2p/security/plaintext/plaintext_connection.hpp>

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define PLAINTEXT_OUTCOME_TRY_VOID_I(var, res, conn, cb) \
  auto && (var) = (res);                                 \
  if ((var).has_error()) {                               \
    closeConnection((conn), (var).error());              \
    cb((var).error());                                   \
    return;                                              \
  }

#define PLAINTEXT_OUTCOME_TRY_NAME_I(var, val, res, conn, cb) \
  PLAINTEXT_OUTCOME_TRY_VOID_I(var, res, conn, cb)            \
  auto && (val) = (var).value();

#define PLAINTEXT_OUTCOME_TRY(name, res, conn, cb) \
  PLAINTEXT_OUTCOME_TRY_NAME_I(UNIQUE_NAME(name), name, res, conn, cb)

#define PLAINTEXT_OUTCOME_VOID_TRY(res, conn, cb) \
  PLAINTEXT_OUTCOME_TRY_VOID_I(UNIQUE_NAME(void_var), res, conn, cb)

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security, Plaintext::Error, e) {
  using E = libp2p::security::Plaintext::Error;
  switch (e) {
    case E::EXCHANGE_SEND_ERROR:
      return "Error occured while sending Exchange message to the peer";
    case E::EXCHANGE_RECEIVE_ERROR:
      return "Error occurred while receiving Exchange message to the peer";
    case E::INVALID_PEER_ID:
      return "Received peer id doesn't match actual peer id";
    case E::EMPTY_PEER_ID:
      return "Peer Id isn't present in the remote peer multiaddr";
  }
  return "Unknown error";
}

namespace libp2p::security {

  Plaintext::Plaintext(
      std::shared_ptr<plaintext::ExchangeMessageMarshaller> marshaller,
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : marshaller_(std::move(marshaller)),
        idmgr_(std::move(idmgr)),
        key_marshaller_{std::move(key_marshaller)} {
    BOOST_ASSERT(marshaller_);
    BOOST_ASSERT(idmgr_);
    BOOST_ASSERT(key_marshaller_);
  }

  peer::Protocol Plaintext::getProtocolId() const {
    // TODO(akvinikym) 29.05.19: think about creating SecurityProtocolRegister
    return "/plaintext/2.0.0";
  }

  void Plaintext::secureInbound(
      std::shared_ptr<connection::RawConnection> inbound,
      SecConnCallbackFunc cb) {
    SL_DEBUG(log_, "securing inbound connection");
    auto rw = std::make_shared<basic::ProtobufMessageReadWriter>(inbound);
    sendExchangeMsg(inbound, rw, cb);
    receiveExchangeMsg(inbound, rw, boost::none, cb);
  }

  void Plaintext::secureOutbound(
      std::shared_ptr<connection::RawConnection> outbound,
      const peer::PeerId &p, SecConnCallbackFunc cb) {
    SL_DEBUG(log_, "securing outbound connection");
    auto rw = std::make_shared<basic::ProtobufMessageReadWriter>(outbound);
    sendExchangeMsg(outbound, rw, cb);
    receiveExchangeMsg(outbound, rw, p, cb);
  }

  void Plaintext::sendExchangeMsg(
      const std::shared_ptr<connection::RawConnection> &conn,
      const std::shared_ptr<basic::ProtobufMessageReadWriter> &rw,
      SecConnCallbackFunc cb) const {
    plaintext::ExchangeMessage exchange_msg{
        .pubkey = idmgr_->getKeyPair().publicKey, .peer_id = idmgr_->getId()};

    // TODO(107): Reentrancy

    PLAINTEXT_OUTCOME_TRY(proto_exchange_msg,
                          marshaller_->handyToProto(exchange_msg), conn, cb)

    rw->write<plaintext::protobuf::Exchange>(
        proto_exchange_msg,
        [self{shared_from_this()}, cb{std::move(cb)}, conn](auto &&res) {
          if (res.has_error()) {
            self->closeConnection(conn, Error::EXCHANGE_SEND_ERROR);
            return cb(Error::EXCHANGE_SEND_ERROR);
          }
        });
  }

  void Plaintext::receiveExchangeMsg(
      const std::shared_ptr<connection::RawConnection> &conn,
      const std::shared_ptr<basic::ProtobufMessageReadWriter> &rw,
      const MaybePeerId &p, SecConnCallbackFunc cb) const {
    auto remote_peer_exchange_bytes = std::make_shared<std::vector<uint8_t>>();
    rw->read<plaintext::protobuf::Exchange>(
        [self{shared_from_this()}, conn, p, cb{std::move(cb)},
         remote_peer_exchange_bytes](auto &&res) {
          if (!res) {
            return cb(res.error());
          }
          self->readCallback(conn, p, cb, remote_peer_exchange_bytes,
                             remote_peer_exchange_bytes->size());
        },
        remote_peer_exchange_bytes);
  }

  void Plaintext::readCallback(
      const std::shared_ptr<connection::RawConnection> &conn,
      const MaybePeerId &p, const SecConnCallbackFunc &cb,
      const std::shared_ptr<std::vector<uint8_t>> &read_bytes,
      outcome::result<size_t> read_call_res) const {
    /*
     * The method does redundant unmarshalling of message bytes to proto
     * exchange message. This could be a subject of further improvement.
     */
    PLAINTEXT_OUTCOME_VOID_TRY(read_call_res, conn, cb);
    PLAINTEXT_OUTCOME_TRY(in_exchange_msg, marshaller_->unmarshal(*read_bytes),
                          conn, cb);
    auto &msg = in_exchange_msg.first;
    auto received_pid = msg.peer_id;
    auto pkey = msg.pubkey;

    // PeerId is derived from the Protobuf-serialized public key, not a raw one
    auto derived_pid_res = peer::PeerId::fromPublicKey(in_exchange_msg.second);
    if (!derived_pid_res) {
      log_->error("cannot create a PeerId from the received public key: {}",
                  derived_pid_res.error().message());
      return cb(derived_pid_res.error());
    }
    auto derived_pid = std::move(derived_pid_res.value());

    if (received_pid != derived_pid) {
      log_->error(
          "ID, which was received in the message ({}) and the one, which was "
          "derived from the public key ({}), differ",
          received_pid.toBase58(), derived_pid.toBase58());
      closeConnection(conn, Error::INVALID_PEER_ID);
      return cb(Error::INVALID_PEER_ID);
    }
    if (p.has_value()) {
      if (received_pid != p.value()) {
        auto s = p.value().toBase58();
        log_->error("received_pid={}, p.value()={}", received_pid.toBase58(),
                    s);
        closeConnection(conn, Error::INVALID_PEER_ID);
        return cb(Error::INVALID_PEER_ID);
      }
    }

    cb(std::make_shared<connection::PlaintextConnection>(
        conn, idmgr_->getKeyPair().publicKey, std::move(pkey),
        key_marshaller_));
  }

  void Plaintext::closeConnection(
      const std::shared_ptr<libp2p::connection::RawConnection> &conn,
      const std::error_code &err) const {
    log_->error("error happened while establishing a Plaintext session: {}",
                err.message());
    if (auto close_res = conn->close(); !close_res) {
      log_->error("connection close attempt ended with error: {}",
                  close_res.error().message());
    }
  }

}  // namespace libp2p::security
