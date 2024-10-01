/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/handshake.hpp>
#include <libp2p/security/noise/handshake_message_marshaller_impl.hpp>
#include <libp2p/security/noise/noise.hpp>

#include <log_conn.hpp>

namespace libp2p::security {
  inline void wrap(bool out,
                   const connection::LayerConnection &conn,
                   SecurityAdaptor::SecConnCallbackFunc &cb) {
    auto [conn_id, op] = log_conn::op::noise(conn.conn_id_.value());
    cb = [=, cb{std::move(cb)}, op{std::move(op)}](
             outcome::result<std::shared_ptr<connection::SecureConnection>> r) {
      if (not r and r.error() == std::errc::bad_address) {
        log_conn::op::noise_mismatch(conn_id);
      }
      op(r);
      log_conn::metrics::noise[out]((bool)r);
      if (r) {
        r.value()->conn_id_ = conn_id;
      }
      cb(std::move(r));
    };
  }

  peer::ProtocolName Noise::getProtocolId() const {
    return kProtocolId;
  }

  Noise::Noise(
      crypto::KeyPair local_key,
      std::shared_ptr<crypto::CryptoProvider> crypto_provider,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : local_key_{std::move(local_key)},
        crypto_provider_{std::move(crypto_provider)},
        key_marshaller_{std::move(key_marshaller)} {}

  void Noise::secureInbound(
      std::shared_ptr<connection::LayerConnection> inbound,
      SecurityAdaptor::SecConnCallbackFunc cb) {
    log_->info("securing inbound connection");
    auto noise_marshaller =
        std::make_unique<noise::HandshakeMessageMarshallerImpl>(
            key_marshaller_);
    wrap(false, *inbound, cb);
    auto handshake =
        std::make_shared<noise::Handshake>(crypto_provider_,
                                           std::move(noise_marshaller),
                                           local_key_,
                                           inbound,
                                           false,
                                           boost::none,
                                           std::move(cb),
                                           key_marshaller_);
    handshake->connect();
  }

  void Noise::secureOutbound(
      std::shared_ptr<connection::LayerConnection> outbound,
      const peer::PeerId &p,
      SecurityAdaptor::SecConnCallbackFunc cb) {
    log_->info("securing outbound connection");
    auto noise_marshaller =
        std::make_unique<noise::HandshakeMessageMarshallerImpl>(
            key_marshaller_);
    wrap(true, *outbound, cb);
    auto handshake =
        std::make_shared<noise::Handshake>(crypto_provider_,
                                           std::move(noise_marshaller),
                                           local_key_,
                                           outbound,
                                           true,
                                           p,
                                           std::move(cb),
                                           key_marshaller_);
    handshake->connect();
  }
}  // namespace libp2p::security
