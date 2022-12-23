/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_CONNECTION_HPP
#define LIBP2P_SECIO_CONNECTION_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/secure_connection.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/log/logger.hpp>

namespace libp2p::crypto {
  namespace aes {
    class AesCtr;
  }
  namespace hmac {
    class HmacProvider;
  }
}  // namespace libp2p::crypto

namespace libp2p::connection {

  /// SECIO connection implementation
  class SecioConnection : public SecureConnection,
                          public std::enable_shared_from_this<SecioConnection> {
   public:
    enum class Error {
      CONN_NOT_INITIALIZED = 1,
      CONN_ALREADY_INITIALIZED,
      INITALIZATION_FAILED,
      UNSUPPORTED_CIPHER,
      UNSUPPORTED_HASH,
      INVALID_MAC,
      TOO_SHORT_BUFFER,
      NOTHING_TO_READ,
      STREAM_IS_BROKEN,
      OVERSIZED_FRAME,
    };

    /**
     * Maximum size of raw SECIO frame.
     * It includes only lengths of encrypted payload and mac signature.
     * It does NOT include 4 bytes of frame length marker.
     */
    static constexpr auto kMaxFrameSize = 8 * 1024 * 1024;
    static constexpr auto kLenMarkerSize = sizeof(uint32_t);

    template <typename SecretType>
    struct AesSecrets {
      SecretType local;
      SecretType remote;
    };

    SecioConnection(
        std::shared_ptr<LayerConnection> original_connection,
        std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
        crypto::PublicKey local_pubkey, crypto::PublicKey remote_pubkey,
        crypto::common::HashType hash_type,
        crypto::common::CipherType cipher_type,
        crypto::StretchedKey local_stretched_key,
        crypto::StretchedKey remote_stretched_key);

    ~SecioConnection() override = default;

    /**
     * Initializer. Have to be called next to constructor before anything else.
     * @return nothing when no errors occurred, an error - otherwise
     */
    outcome::result<void> init();

    /**
     * Checks whether connection state is initialized.
     * @return true when initialized, otherwise - false
     */
    bool isInitialized() const;

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

    bool isClosed() const override;

    outcome::result<void> close() override;

   private:
    /**
     * Retrieves the next available SECIO message from the network.
     */
    void readNextMessage(ReadCallbackFunc cb);

    /**
     * Moves decrypted bytes from internal buffer to the output buffer.
     * Does no boundary checks.
     * @param out - buffer to be filled with decrypted bytes
     * @param bytes - amount of bytes to pop from internal buffer
     */
    void popUserData(gsl::span<uint8_t> out, size_t bytes);

    /**
     * Computes MAC digest to sign a message using local peer key
     * @param message bytes to be signed
     * @return signature bytes or an error if happened
     */
    outcome::result<common::ByteArray> macLocal(
        gsl::span<const uint8_t> message) const;

    /**
     * Computes MAC digest to sign a message using remote peer key
     * @param message bytes to be signed
     * @return signature bytes or an error if happened
     */
    outcome::result<common::ByteArray> macRemote(
        gsl::span<const uint8_t> message) const;

    /// Returns MAC digest size in bytes for the chosen algorithm
    outcome::result<size_t> macSize() const;

    std::shared_ptr<LayerConnection> original_connection_;
    std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider_;
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;

    crypto::PublicKey local_;
    crypto::PublicKey remote_;

    crypto::common::HashType hash_type_;
    crypto::common::CipherType cipher_type_;
    crypto::StretchedKey local_stretched_key_;
    crypto::StretchedKey remote_stretched_key_;

    boost::optional<AesSecrets<crypto::common::Aes128Secret>> aes128_secrets_;
    boost::optional<AesSecrets<crypto::common::Aes256Secret>> aes256_secrets_;

    boost::optional<std::unique_ptr<crypto::aes::AesCtr>> local_encryptor_;
    boost::optional<std::unique_ptr<crypto::aes::AesCtr>> remote_decryptor_;

    std::queue<uint8_t> user_data_buffer_;

    std::shared_ptr<common::ByteArray> read_buffer_;

    log::Logger log_ = log::createLogger("SecIoConnection");

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(
        libp2p::connection::SecioConnection);
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, SecioConnection::Error);

#endif  // LIBP2P_SECIO_CONNECTION_HPP
