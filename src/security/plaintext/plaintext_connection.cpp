/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/plaintext/plaintext_connection.hpp>

#include <boost/assert.hpp>
#include "libp2p/crypto/protobuf/protobuf_key.hpp"

namespace libp2p::connection {

  PlaintextConnection::PlaintextConnection(
      std::shared_ptr<LayerConnection> original_connection,
      crypto::PublicKey localPubkey, crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : original_connection_{std::move(original_connection)},
        local_(std::move(localPubkey)),
        remote_(std::move(remotePubkey)),
        key_marshaller_{std::move(key_marshaller)} {
    BOOST_ASSERT(original_connection_);
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
    return original_connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::localMultiaddr() {
    return original_connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::remoteMultiaddr() {
    return original_connection_->remoteMultiaddr();
  }

  void PlaintextConnection::read(gsl::span<uint8_t> in, size_t bytes,
                                 Reader::ReadCallbackFunc f) {
    return original_connection_->read(in, bytes, std::move(f));
  };

  void PlaintextConnection::readSome(gsl::span<uint8_t> in, size_t bytes,
                                     Reader::ReadCallbackFunc f) {
    return original_connection_->readSome(in, bytes, std::move(f));
  };

  void PlaintextConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                                  Writer::WriteCallbackFunc f) {
    return original_connection_->write(in, bytes, std::move(f));
  }

  void PlaintextConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                      Writer::WriteCallbackFunc f) {
    return original_connection_->writeSome(in, bytes, std::move(f));
  }

  void PlaintextConnection::deferReadCallback(outcome::result<size_t> res,
                                              ReadCallbackFunc cb) {
    original_connection_->deferReadCallback(res, std::move(cb));
  }

  void PlaintextConnection::deferWriteCallback(std::error_code ec,
                                               WriteCallbackFunc cb) {
    original_connection_->deferWriteCallback(ec, std::move(cb));
  }

  bool PlaintextConnection::isClosed() const {
    return original_connection_->isClosed();
  }

  outcome::result<void> PlaintextConnection::close() {
    return original_connection_->close();
  }
}  // namespace libp2p::connection
