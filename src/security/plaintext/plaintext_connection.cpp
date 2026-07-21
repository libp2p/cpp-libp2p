/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/plaintext/plaintext_connection.hpp>

#include <boost/assert.hpp>
#include <libp2p/crypto/protobuf/protobuf_key.hpp>

namespace libp2p::connection {

  PlaintextConnection::PlaintextConnection(
      std::shared_ptr<LayerConnection> original_connection,
      crypto::PublicKey localPubkey,
      crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : original_connection_{std::move(original_connection)},
        local_(std::move(localPubkey)),
        remote_(std::move(remotePubkey)),
        key_marshaller_{std::move(key_marshaller)} {
    BOOST_ASSERT(original_connection_);
    BOOST_ASSERT(key_marshaller_);
  }

  PollOutcome<size_t> PlaintextConnection::pollReadSome(PollWaker waker,
                                                        BytesOut buffer) {
    return original_connection_->pollReadSome(waker, buffer);
  }

  PollOutcome<size_t> PlaintextConnection::pollWriteSome(PollWaker waker,
                                                         BytesIn buffer) {
    return original_connection_->pollWriteSome(waker, buffer);
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

  bool PlaintextConnection::isInitiator() const {
    return original_connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::localMultiaddr() {
    return original_connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::remoteMultiaddr() {
    return original_connection_->remoteMultiaddr();
  }

  bool PlaintextConnection::isClosed() const {
    return original_connection_->isClosed();
  }

  outcome::result<void> PlaintextConnection::close() {
    return original_connection_->close();
  }
}  // namespace libp2p::connection
