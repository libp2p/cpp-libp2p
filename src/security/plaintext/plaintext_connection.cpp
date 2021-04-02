/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/plaintext/plaintext_connection.hpp>

#include <boost/assert.hpp>
#include "libp2p/crypto/protobuf/protobuf_key.hpp"

namespace libp2p::connection {

  PlaintextConnection::PlaintextConnection(
      std::shared_ptr<RawConnection> raw_connection,
      crypto::PublicKey localPubkey, crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : raw_connection_{std::move(raw_connection)},
        local_(std::move(localPubkey)),
        remote_(std::move(remotePubkey)),
        key_marshaller_{std::move(key_marshaller)} {
    BOOST_ASSERT(raw_connection_);
    BOOST_ASSERT(key_marshaller_);
  }

  outcome::result<peer::PeerId> PlaintextConnection::localPeer() const {
    auto proto_local_key_res = key_marshaller_->marshal(local_);
    if (!proto_local_key_res) {
      return proto_local_key_res.error();
    }
    return peer::PeerId::fromPublicKey(proto_local_key_res.value());
  }

  outcome::result<peer::PeerId> PlaintextConnection::remotePeer() const {
    auto proto_remote_key_res = key_marshaller_->marshal(remote_);
    if (!proto_remote_key_res) {
      return proto_remote_key_res.error();
    }
    return peer::PeerId::fromPublicKey(proto_remote_key_res.value());
  }

  outcome::result<crypto::PublicKey> PlaintextConnection::remotePublicKey()
      const {
    return remote_;
  }

  bool PlaintextConnection::isInitiator() const noexcept {
    return raw_connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::localMultiaddr() {
    return raw_connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::remoteMultiaddr() {
    return raw_connection_->remoteMultiaddr();
  }

  void PlaintextConnection::read(gsl::span<uint8_t> in, size_t bytes,
                                 Reader::ReadCallbackFunc f) {
    return raw_connection_->read(in, bytes, std::move(f));
  };

  void PlaintextConnection::readSome(gsl::span<uint8_t> in,
                                     size_t bytes,
                                     Reader::ReadCallbackFunc f) {
    return raw_connection_->readSome(in, bytes, std::move(f));
  };

  void PlaintextConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                                  Writer::WriteCallbackFunc f) {
    return raw_connection_->write(in, bytes, std::move(f));
  }

  void PlaintextConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                      Writer::WriteCallbackFunc f) {
    return raw_connection_->writeSome(in, bytes, std::move(f));
  }

  void PlaintextConnection::deferReadCallback(outcome::result<size_t> res,
                                         ReadCallbackFunc cb) {
    raw_connection_->deferReadCallback(res, std::move(cb));
  }

  void PlaintextConnection::deferWriteCallback(std::error_code ec,
                                          WriteCallbackFunc cb) {
    raw_connection_->deferWriteCallback(ec, std::move(cb));
  }

  bool PlaintextConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> PlaintextConnection::close() {
    return raw_connection_->close();
  }
}  // namespace libp2p::connection
