/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <optional>

#include <libp2p/connection/secure_connection.hpp>
#include <libp2p/crypto/key_marshaller.hpp>

namespace libp2p::connection {
  class PlaintextConnection : public SecureConnection {
   public:
    PlaintextConnection(
        std::shared_ptr<LayerConnection> original_connection,
        crypto::PublicKey localPubkey,
        crypto::PublicKey remotePubkey,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    ~PlaintextConnection() override = default;

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    void read(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;

    void readSome(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    void write(BytesIn in, size_t bytes, WriteCallbackFunc cb) override;

    void writeSome(BytesIn in, size_t bytes, WriteCallbackFunc cb) override;

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

    bool isClosed() const override;

    outcome::result<void> close() override;

   private:
    std::shared_ptr<LayerConnection> original_connection_;

    crypto::PublicKey local_;
    crypto::PublicKey remote_;

    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;
  };
}  // namespace libp2p::connection
