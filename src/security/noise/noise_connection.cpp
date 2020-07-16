/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/noise_connection.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, NoiseConnection::Error, e) {
  using E = libp2p::connection::NoiseConnection::Error;
  switch (e) {
    case E::FAILURE:
      return "failure";
    default:
      return "Unknown error";
  }
}

namespace libp2p::connection {
  NoiseConnection::NoiseConnection(
      std::shared_ptr<RawConnection> raw_connection,
      crypto::PublicKey localPubkey, crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : raw_connection_{std::move(raw_connection)},
        local_{std::move(localPubkey)},
        remote_{std::move(remotePubkey)},
        key_marshaller_{std::move(key_marshaller)} {
    BOOST_ASSERT(raw_connection_);
    BOOST_ASSERT(key_marshaller_);
  }

  bool NoiseConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> NoiseConnection::close() {
    // todo
    return outcome::success();
  }

  void NoiseConnection::read(gsl::span<uint8_t> out, size_t bytes,
                             libp2p::basic::Reader::ReadCallbackFunc cb) {}

  void NoiseConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                 libp2p::basic::Reader::ReadCallbackFunc cb) {}

  void NoiseConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                              libp2p::basic::Writer::WriteCallbackFunc cb) {}

  void NoiseConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                  libp2p::basic::Writer::WriteCallbackFunc cb) {
  }

  bool NoiseConnection::isInitiator() const noexcept {
    return raw_connection_->isInitiator();
  }

  outcome::result<libp2p::multi::Multiaddress>
  NoiseConnection::localMultiaddr() {
    return raw_connection_->localMultiaddr();
  }

  outcome::result<libp2p::multi::Multiaddress>
  NoiseConnection::remoteMultiaddr() {
    return raw_connection_->remoteMultiaddr();
  }

  outcome::result<libp2p::peer::PeerId> NoiseConnection::localPeer() const {
    OUTCOME_TRY(proto_local_key, key_marshaller_->marshal(local_));
    return peer::PeerId::fromPublicKey(proto_local_key);
  }

  outcome::result<libp2p::peer::PeerId> NoiseConnection::remotePeer() const {
    OUTCOME_TRY(proto_remote_key, key_marshaller_->marshal(remote_));
    return peer::PeerId::fromPublicKey(proto_remote_key);
  }

  outcome::result<libp2p::crypto::PublicKey> NoiseConnection::remotePublicKey()
      const {
    return remote_;
  }

}  // namespace libp2p::connection
