/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_DIALER_HPP
#define LIBP2P_SECIO_DIALER_HPP

#include <boost/optional.hpp>
#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/key.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/security/secio/exchange_message.hpp>
#include <libp2p/security/secio/propose_message.hpp>
#include <libp2p/security/secio/propose_message_marshaller.hpp>

namespace libp2p::basic {
  class ProtobufMessageReadWriter;
}

namespace libp2p::security::secio {

  /// Helper class for establishing SECIO connection
  class Dialer {
   public:
    /// protobuf messages read-writer in/to the wire
    const std::shared_ptr<libp2p::basic::ProtobufMessageReadWriter> rw;

    enum class Error {
      INTERNAL_FAILURE = 1,
      PEER_COMMUNICATING_ITSELF,
      NO_COMMON_EC_ALGO,
      NO_COMMON_CIPHER_ALGO,
      NO_COMMON_HASH_ALGO,
    };

    struct Algorithm {
      crypto::common::CurveType curve;
      crypto::common::CipherType cipher;
      crypto::common::HashType hash;
    };

    Dialer(std::shared_ptr<connection::RawConnection> connection);

    /// Stores byte-exact copy of SECIO proposal sent to remote peer
    void storeLocalPeerProposalBytes(
        const std::shared_ptr<std::vector<uint8_t>> &bytes);

    /// Stores byte-exact copy of SECIO proposal received from remote peer
    void storeRemotePeerProposalBytes(
        const std::shared_ptr<std::vector<uint8_t>> &bytes);

    /// Stores ephemeral keypair for further computations
    void storeEphemeralKeypair(crypto::EphemeralKeyPair keypair);

    /**
     * Stores a pair of stretched keys to assign them later to local and remote
     * peers
     */
    void storeStretchedKeys(
        std::pair<crypto::StretchedKey, crypto::StretchedKey> keys);

    /**
     * Produces a corpus to be signed during SECIO keys exchange phase.
     * @param for_local_peer - true if the corpus is going to be signed by local
     * peer, false - remote peer
     * @param ephemeral_public_key - epubkey bytes of the corresponding peer
     * (local or remote)
     * @return byte sequence or an error if happened
     */
    outcome::result<std::vector<uint8_t>> getCorpus(
        bool for_local_peer,
        gsl::span<const uint8_t> ephemeral_public_key) const;

    /**
     * Computes which cipher, hash, and ec-curve to use considering the
     * knowledge which peer set is preferred.
     * @param local - propose message sent to the remote peer
     * @param remote - propose message received from the remote peer
     * @return a structure with chosen algorithms or an error if happened
     */
    outcome::result<Algorithm> determineCommonAlgorithm(
        const ProposeMessage &local, const ProposeMessage &remote);

    /// Returns common ec-curve type if already defined
    outcome::result<crypto::common::CurveType> chosenCurve() const;

    /// Returns common cipher algorithm if already defined
    outcome::result<crypto::common::CipherType> chosenCipher() const;

    /// Returns common hash algorithm if already defined
    outcome::result<crypto::common::HashType> chosenHash() const;

    /**
     * Retrieves public key of a remote peer in an unmarshalled form.
     * @param key_marshaller to unmarshal key bytes
     * @param propose_marshaller to unmarshal remote peer's proposal message
     * bytes
     * @return remote peer public key or an error if happened
     */
    outcome::result<crypto::PublicKey> remotePublicKey(
        const std::shared_ptr<crypto::marshaller::KeyMarshaller>
            &key_marshaller,
        const std::shared_ptr<ProposeMessageMarshaller> &propose_marshaller)
        const;

    /// Computes shared secret via ec-cryptography
    outcome::result<crypto::Buffer> generateSharedSecret(
        crypto::Buffer remote_ephemeral_public_key) const;

    /// Defines which stretched key is for the local peer
    outcome::result<crypto::StretchedKey> localStretchedKey() const;

    /// Defines which stretched key is for the remote peer
    outcome::result<crypto::StretchedKey> remoteStretchedKey() const;

   private:
    /**
     * Computes which peer settings are preferred.
     * @return true if local's peer settings are preferred, false - otherwise
     */
    static outcome::result<bool> determineRoles(const ProposeMessage &local,
                                                const ProposeMessage &remote);

    /**
     * Determine common set of settings considering given peers' preference.
     * @return chosen algorithms structure if successful
     */
    static outcome::result<Algorithm> findCommonAlgo(
        const ProposeMessage &local, const ProposeMessage &remote,
        bool local_peer_is_preferred);

    boost::optional<std::vector<uint8_t>> local_peer_proposal_bytes_;
    boost::optional<std::vector<uint8_t>> remote_peer_proposal_bytes_;
    boost::optional<Algorithm> chosen_algorithm_;
    boost::optional<bool> local_peer_is_preferred_;
    boost::optional<crypto::EphemeralKeyPair> ekey_pair_;
    boost::optional<std::pair<crypto::StretchedKey, crypto::StretchedKey>>
        stretched_keys_;
  };
}  // namespace libp2p::security::secio

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::secio, Dialer::Error);

#endif  // LIBP2P_SECIO_DIALER_HPP
