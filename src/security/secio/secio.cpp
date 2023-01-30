/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/secio/secio.hpp>

#include <generated/security/secio/protobuf/secio.pb.h>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/security/error.hpp>
#include <libp2p/security/secio/secio_connection.hpp>
#include <libp2p/security/secio/secio_dialer.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define SECIO_OUTCOME_TRY_VOID_I(var, res, conn, cb) \
  auto && (var) = (res);                             \
  if ((var).has_error()) {                           \
    self->closeConnection((conn), (var).error());    \
    cb((var).error());                               \
    return;                                          \
  }

#define SECIO_OUTCOME_TRY_NAME_I(var, val, res, conn, cb) \
  SECIO_OUTCOME_TRY_VOID_I(var, res, conn, cb)            \
  auto && (val) = (var).value();

#define SECIO_OUTCOME_TRY(name, res, conn, cb) \
  SECIO_OUTCOME_TRY_NAME_I(UNIQUE_NAME(name), name, res, conn, cb)

#define SECIO_OUTCOME_VOID_TRY(res, conn, cb) \
  SECIO_OUTCOME_TRY_VOID_I(UNIQUE_NAME(void_var), res, conn, cb)

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security, Secio::Error, e) {
  using E = libp2p::security::Secio::Error;
  switch (e) {  // NOLINT
    case E::REMOTE_PEER_SIGNATURE_IS_INVALID:
      return "Remote peer exchange message contains invalid signature";
    case E::INITIAL_PACKET_VERIFICATION_FAILED:
      return "Error happened while initial packet verification";
    default:
      return "Unknown error";
  }
}

namespace libp2p::security {

  Secio::Secio(
      std::shared_ptr<crypto::random::CSPRNG> csprng,
      std::shared_ptr<crypto::CryptoProvider> crypto_provider,
      std::shared_ptr<secio::ProposeMessageMarshaller> propose_marshaller,
      std::shared_ptr<secio::ExchangeMessageMarshaller> exchange_marshaller,
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider)
      : csprng_(std::move(csprng)),
        crypto_provider_(std::move(crypto_provider)),
        propose_marshaller_(std::move(propose_marshaller)),
        exchange_marshaller_(std::move(exchange_marshaller)),
        idmgr_(std::move(idmgr)),
        key_marshaller_(std::move(key_marshaller)),
        hmac_provider_(std::move(hmac_provider)),
        propose_message_{.rand = csprng_->randomBytes(16),
                         .pubkey = {},  // marshalled public key will be stored
                                        // here, initialized in constructor body
                         .exchanges = kExchanges,
                         .ciphers = kCiphers,
                         .hashes = kHashes} {
    BOOST_ASSERT(csprng_);
    BOOST_ASSERT(propose_marshaller_);
    BOOST_ASSERT(exchange_marshaller_);
    BOOST_ASSERT(idmgr_);
    BOOST_ASSERT(key_marshaller_);

    /* Due to weird SECIO protobuf specification, we have to deal with a public
     * key in raw-bytes (marshalled) format. That is a known drawback.
     */
    auto public_key_res{
        key_marshaller_->marshal(idmgr_->getKeyPair().publicKey)};
    BOOST_ASSERT(public_key_res);
    propose_message_.pubkey.swap(public_key_res.value().key);
  }

  peer::ProtocolName Secio::getProtocolId() const {
    return kProtocolId;
  }

  void Secio::secureInbound(
      std::shared_ptr<connection::LayerConnection> inbound,
      SecurityAdaptor::SecConnCallbackFunc cb) {
    log_->info("securing inbound connection");
    auto dialer = std::make_shared<secio::Dialer>(inbound);
    sendProposeMessage(inbound, dialer, cb);
    receiveProposeMessage(inbound, dialer, cb);
  }

  void Secio::secureOutbound(
      std::shared_ptr<connection::LayerConnection> outbound,
      const peer::PeerId &p, SecurityAdaptor::SecConnCallbackFunc cb) {
    log_->info("securing outbound connection");
    auto dialer = std::make_shared<secio::Dialer>(outbound);
    sendProposeMessage(outbound, dialer, cb);
    receiveProposeMessage(outbound, dialer, cb);
  }

  void Secio::sendProposeMessage(
      const std::shared_ptr<connection::LayerConnection> &conn,
      const std::shared_ptr<secio::Dialer> &dialer,
      SecurityAdaptor::SecConnCallbackFunc cb) const {
    auto proto_propose{propose_marshaller_->handyToProto(propose_message_)};
    auto own_proposal_bytes = std::make_shared<std::vector<uint8_t>>();
    dialer->rw->write<secio::protobuf::Propose>(
        proto_propose,
        [self{shared_from_this()}, conn, cb{std::move(cb)}](
            auto &&res) mutable { SECIO_OUTCOME_VOID_TRY(res, conn, cb) },
        own_proposal_bytes);
    dialer->storeLocalPeerProposalBytes(own_proposal_bytes);
    SL_TRACE(log_, "proposal sent");
  }

  void Secio::receiveProposeMessage(
      const std::shared_ptr<connection::LayerConnection> &conn,
      const std::shared_ptr<secio::Dialer> &dialer,
      SecurityAdaptor::SecConnCallbackFunc cb) const {
    auto remote_peer_proposal_bytes = std::make_shared<std::vector<uint8_t>>();
    dialer->rw->read<secio::protobuf::Propose>(
        [self{shared_from_this()}, conn, dialer, cb{std::move(cb)},
         remote_peer_proposal_bytes](auto &&res) {
          SECIO_OUTCOME_TRY(other_peer_proto_propose, res, conn, cb);
          auto remote_peer_propose{self->propose_marshaller_->protoToHandy(
              other_peer_proto_propose)};
          SECIO_OUTCOME_VOID_TRY(
              dialer->determineCommonAlgorithm(self->propose_message_,
                                               remote_peer_propose),
              conn, cb)
          dialer->storeRemotePeerProposalBytes(remote_peer_proposal_bytes);
          SL_TRACE(self->log_, "remote peer proposal received");
          self->remote_peer_rand_.swap(remote_peer_propose.rand);
          self->sendExchangeMessage(conn, dialer, cb);
        },
        remote_peer_proposal_bytes);
  }

  void Secio::sendExchangeMessage(
      const std::shared_ptr<connection::LayerConnection> &conn,
      const std::shared_ptr<secio::Dialer> &dialer,
      SecurityAdaptor::SecConnCallbackFunc cb) const {
    const auto &&self{this};
    SECIO_OUTCOME_TRY(curve, dialer->chosenCurve(), conn, cb)
    SECIO_OUTCOME_TRY(ephemeral_key,
                      crypto_provider_->generateEphemeralKeyPair(curve), conn,
                      cb)
    dialer->storeEphemeralKeypair(ephemeral_key);
    SECIO_OUTCOME_TRY(
        local_corpus,
        dialer->getCorpus(true, ephemeral_key.ephemeral_public_key), conn, cb)
    SECIO_OUTCOME_TRY(
        local_corpus_signature,
        crypto_provider_->sign(local_corpus, idmgr_->getKeyPair().privateKey),
        conn, cb)
    secio::ExchangeMessage local_exchange{
        .epubkey = ephemeral_key.ephemeral_public_key,
        .signature = std::move(local_corpus_signature)};
    auto proto_exchange{exchange_marshaller_->handyToProto(local_exchange)};
    dialer->rw->write<secio::protobuf::Exchange>(
        proto_exchange,
        [self{shared_from_this()}, conn, dialer,
         cb{std::move(cb)}](auto &&res) {
          SECIO_OUTCOME_VOID_TRY(res, conn, cb)
          SL_TRACE(self->log_, "exchange message sent");
          self->receiveExchangeMessage(conn, dialer, cb);
        });
  }

  void Secio::receiveExchangeMessage(
      const std::shared_ptr<connection::LayerConnection> &conn,
      const std::shared_ptr<secio::Dialer> &dialer,
      SecurityAdaptor::SecConnCallbackFunc cb) const {
    dialer->rw->read<secio::protobuf::Exchange>(
        [self{shared_from_this()}, conn, dialer,
         cb{std::move(cb)}](auto &&res) {
          SECIO_OUTCOME_TRY(remote_proto_exchange, res, conn, cb)
          auto remote_exchange{
              self->exchange_marshaller_->protoToHandy(remote_proto_exchange)};
          SL_TRACE(self->log_, "remote exchange message received");
          SECIO_OUTCOME_TRY(remote_corpus,
                            dialer->getCorpus(false, remote_exchange.epubkey),
                            conn, cb)

          SECIO_OUTCOME_TRY(remote_key,
                            dialer->remotePublicKey(self->key_marshaller_,
                                                    self->propose_marshaller_),
                            conn, cb)
          SECIO_OUTCOME_TRY(
              verify_res,
              self->crypto_provider_->verify(
                  remote_corpus, remote_exchange.signature, remote_key),
              conn, cb)
          if (!verify_res) {
            const auto error{Error::REMOTE_PEER_SIGNATURE_IS_INVALID};
            self->closeConnection(conn, error);
            cb(error);
            return;
          }

          SECIO_OUTCOME_TRY(
              shared_secret,
              dialer->generateSharedSecret(remote_exchange.epubkey), conn, cb)
          SECIO_OUTCOME_TRY(chosen_cipher, dialer->chosenCipher(), conn, cb)
          SECIO_OUTCOME_TRY(chosen_hash, dialer->chosenHash(), conn, cb)
          SECIO_OUTCOME_TRY(stretched_keys,
                            self->crypto_provider_->stretchKey(
                                chosen_cipher, chosen_hash, shared_secret),
                            conn, cb)
          dialer->storeStretchedKeys(std::move(stretched_keys));

          SECIO_OUTCOME_TRY(remote_pubkey,
                            dialer->remotePublicKey(self->key_marshaller_,
                                                    self->propose_marshaller_),
                            conn, cb)
          SECIO_OUTCOME_TRY(local_stretched_key, dialer->localStretchedKey(),
                            conn, cb)
          SECIO_OUTCOME_TRY(remote_stretched_key, dialer->remoteStretchedKey(),
                            conn, cb)

          auto secio_conn = std::make_shared<connection::SecioConnection>(
              conn, self->hmac_provider_, self->key_marshaller_,
              self->idmgr_->getKeyPair().publicKey, remote_pubkey, chosen_hash,
              chosen_cipher, local_stretched_key, remote_stretched_key);
          SECIO_OUTCOME_VOID_TRY(secio_conn->init(), conn, cb)
          secio_conn->write(
              self->remote_peer_rand_, self->remote_peer_rand_.size(),
              [self, conn, cb, secio_conn](auto &&write_res) {
                SECIO_OUTCOME_TRY(written_bytes, write_res, conn, cb)
                if (written_bytes != self->remote_peer_rand_.size()) {
                  return cb(Error::INITIAL_PACKET_VERIFICATION_FAILED);
                }
                const auto kToRead{self->propose_message_.rand.size()};
                auto buffer = std::make_shared<common::ByteArray>(kToRead);
                secio_conn->read(
                    *buffer, kToRead,
                    [self, cb, conn, secio_conn, buffer](auto &&read_res) {
                      SECIO_OUTCOME_TRY(read_bytes, read_res, conn, cb)
                      if (read_bytes != buffer->size()
                          or *buffer != self->propose_message_.rand) {
                        return cb(Error::INITIAL_PACKET_VERIFICATION_FAILED);
                      }
                      SL_TRACE(self->log_, "connection initialized");
                      cb(secio_conn);
                    });
              });
        });
  }

  void Secio::closeConnection(
      const std::shared_ptr<libp2p::connection::LayerConnection> &conn,
      const std::error_code &err) const {
    log_->error("error happened, closing connection: {}", err.message());
    if (auto close_res = conn->close(); !close_res) {
      log_->error("connection close attempt ended with error: {}",
                  close_res.error().message());
    }
  }

}  // namespace libp2p::security
