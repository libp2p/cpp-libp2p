/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/handshake.hpp>
#include <libp2p/security/noise/handshake_coro.hpp>
#include <libp2p/security/noise/handshake_message_marshaller_impl.hpp>
#include <libp2p/security/noise/noise.hpp>

namespace libp2p::security {
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

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::SecureConnection>>>
  Noise::secureInboundCoro(std::shared_ptr<connection::LayerConnection> inbound) {
    log_->info("securing inbound connection (coroutine)");
    auto noise_marshaller =
        std::make_unique<noise::HandshakeMessageMarshallerImpl>(
            key_marshaller_);
    auto handshake =
        std::make_shared<noise::HandshakeCoro>(crypto_provider_,
                                            std::move(noise_marshaller),
                                            local_key_,
                                            inbound,
                                            false,
                                            boost::none,
                                            key_marshaller_);
    co_return co_await handshake->connect();
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::SecureConnection>>>
  Noise::secureOutboundCoro(std::shared_ptr<connection::LayerConnection> outbound,
                          const peer::PeerId &p) {
    log_->info("securing outbound connection (coroutine)");
    auto noise_marshaller =
        std::make_unique<noise::HandshakeMessageMarshallerImpl>(
            key_marshaller_);
    auto handshake =
        std::make_shared<noise::HandshakeCoro>(crypto_provider_,
                                            std::move(noise_marshaller),
                                            local_key_,
                                            outbound,
                                            true,
                                            p,
                                            key_marshaller_);
    co_return co_await handshake->connect();
  }
}  // namespace libp2p::security
