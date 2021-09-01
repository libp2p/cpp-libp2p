/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/secio/secio_dialer.hpp>

#include <generated/security/secio/protobuf/secio.pb.h>
#include <libp2p/basic/message_read_writer_bigendian.hpp>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/security/secio/exchange_message_marshaller.hpp>
#include <libp2p/security/secio/propose_message_marshaller.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::secio, Dialer::Error, e) {
  using E = libp2p::security::secio::Dialer::Error;
  switch (e) {
    case E::INTERNAL_FAILURE:
      return "SECIO connection cannot be established due to an error in Dialer";
    case E::PEER_COMMUNICATING_ITSELF:
      return "Propose messages are equal, the peer is communicating itself";
    case E::NO_COMMON_EC_ALGO:
      return "Peers do not have exchange algorithms in common";
    case E::NO_COMMON_CIPHER_ALGO:
      return "Peers do not have cipher algorithms in common";
    case E::NO_COMMON_HASH_ALGO:
      return "Peers do not have hash algorithms in common";
    default:
      return "Unknown error";
  }
}

namespace {

  /**
   * Searches the first common string term in two sets of values that are
   * provided as comma-separated strings. Searching takes into account which set
   * values are preferred.
   *
   * For example, there are two strings: X = "A,B,C,D,P" and Y = "O,P,Q,B" and X
   * is a preferred set of values. Then "B" would be the result of processing.
   *
   * @param first_set - first set of comma-separated values as string
   * @param second_set - second set of comma-separated values as string
   * @param first_is_preferred - sets which input set is preferred
   * @return a common value. If there is nothing in common between two given
   * inputs, then boost::none will be returned.
   */
  boost::optional<std::string> BestMatch(std::string_view first_set,
                                         std::string_view second_set,
                                         bool first_is_preferred) {
    auto split =
        [](const std::string_view &s) -> std::vector<std::string_view> {
      std::vector<std::string_view> pieces;
      constexpr std::string_view delimiter{","};
      constexpr auto delimiter_length{delimiter.length()};

      auto start{0u};
      auto end{s.find(delimiter)};
      while (end != std::string::npos) {
        pieces.emplace_back(s.substr(start, end - start));
        start = end + delimiter_length;
        end = s.find(delimiter, start);
      }
      pieces.emplace_back(s.substr(start, end));
      return pieces;
    };

    auto preferred{first_is_preferred ? split(first_set) : split(second_set)};
    auto second{first_is_preferred ? split(second_set) : split(first_set)};

    for (const auto &pref_item : preferred) {
      for (const auto &sec_item : second) {
        if (pref_item == sec_item) {
          return std::string(pref_item);
        }
      }
    }

    return boost::none;
  }
}  // namespace

namespace libp2p::security::secio {
  Dialer::Dialer(std::shared_ptr<connection::RawConnection> connection)
      : rw{std::make_shared<libp2p::basic::ProtobufMessageReadWriter>(
          std::make_shared<libp2p::basic::MessageReadWriterBigEndian>(
              std::move(connection)))} {}

  void Dialer::storeLocalPeerProposalBytes(
      const std::shared_ptr<std::vector<uint8_t>> &bytes) {
    local_peer_proposal_bytes_ = *bytes;
  }

  void Dialer::storeRemotePeerProposalBytes(
      const std::shared_ptr<std::vector<uint8_t>> &bytes) {
    remote_peer_proposal_bytes_ = *bytes;
  }

  void Dialer::storeEphemeralKeypair(crypto::EphemeralKeyPair keypair) {
    ekey_pair_ = std::move(keypair);
  }

  void Dialer::storeStretchedKeys(
      std::pair<crypto::StretchedKey, crypto::StretchedKey> keys) {
    stretched_keys_ = std::move(keys);
  }

  outcome::result<Dialer::Algorithm> Dialer::determineCommonAlgorithm(
      const ProposeMessage &local, const ProposeMessage &remote) {
    OUTCOME_TRY(local_peer_is_preferred, determineRoles(local, remote));
    local_peer_is_preferred_ = local_peer_is_preferred;
    OUTCOME_TRY(chosen_algorithm,
                findCommonAlgo(local, remote, local_peer_is_preferred));
    chosen_algorithm_ = chosen_algorithm;
    return chosen_algorithm;
  }

  outcome::result<crypto::common::CurveType> Dialer::chosenCurve() const {
    if (chosen_algorithm_) {
      return chosen_algorithm_->curve;
    }
    return Error::NO_COMMON_EC_ALGO;
  }

  outcome::result<crypto::common::CipherType> Dialer::chosenCipher() const {
    if (chosen_algorithm_) {
      return chosen_algorithm_->cipher;
    }
    return Error::NO_COMMON_CIPHER_ALGO;
  }

  outcome::result<crypto::common::HashType> Dialer::chosenHash() const {
    if (chosen_algorithm_) {
      return chosen_algorithm_->hash;
    }
    return Error::NO_COMMON_HASH_ALGO;
  }

  outcome::result<crypto::PublicKey> Dialer::remotePublicKey(
      const std::shared_ptr<crypto::marshaller::KeyMarshaller> &key_marshaller,
      const std::shared_ptr<ProposeMessageMarshaller> &propose_marshaller)
      const {
    if (!remote_peer_proposal_bytes_) {
      return Error::INTERNAL_FAILURE;
    }

    OUTCOME_TRY(remote_propose,
                propose_marshaller->unmarshal(*remote_peer_proposal_bytes_));
    crypto::ProtobufKey proto_public_key{remote_propose.pubkey};
    OUTCOME_TRY(public_key,
                key_marshaller->unmarshalPublicKey(proto_public_key));

    return std::move(public_key);  // looks like it is legal to move here due to
                                   // outcome::result initialization in between
  }

  outcome::result<crypto::Buffer> Dialer::generateSharedSecret(
      crypto::Buffer remote_ephemeral_public_key) const {
    if (not ekey_pair_) {
      return Error::INTERNAL_FAILURE;
    }
    OUTCOME_TRY(shared_secret,
                ekey_pair_.get().shared_secret_generator(
                    std::move(remote_ephemeral_public_key)));
    return std::move(shared_secret);
  }

  outcome::result<crypto::StretchedKey> Dialer::localStretchedKey() const {
    if (local_peer_is_preferred_ and stretched_keys_) {
      return (*local_peer_is_preferred_ ? stretched_keys_->first
                                        : stretched_keys_->second);
    }
    return Error::INTERNAL_FAILURE;
  }

  outcome::result<crypto::StretchedKey> Dialer::remoteStretchedKey() const {
    if (local_peer_is_preferred_ and stretched_keys_) {
      return (*local_peer_is_preferred_ ? stretched_keys_->second
                                        : stretched_keys_->first);
    }
    return Error::INTERNAL_FAILURE;
  }

  outcome::result<bool> Dialer::determineRoles(const ProposeMessage &local,
                                               const ProposeMessage &remote) {
    // determine preferred peer
    std::vector<uint8_t> corpus1{remote.pubkey.begin(), remote.pubkey.end()};
    std::copy(local.rand.begin(), local.rand.end(),
              std::back_inserter(corpus1));

    std::vector<uint8_t> corpus2{local.pubkey.begin(), local.pubkey.end()};
    std::copy(remote.rand.begin(), remote.rand.end(),
              std::back_inserter(corpus2));

    OUTCOME_TRY(oh1, crypto::sha256(corpus1));
    OUTCOME_TRY(oh2, crypto::sha256(corpus2));

    if (oh1 == oh2) {
      return Error::PEER_COMMUNICATING_ITSELF;
    }

    bool local_is_preferred{oh1 > oh2};
    return local_is_preferred;
  }

  outcome::result<Dialer::Algorithm> Dialer::findCommonAlgo(
      const ProposeMessage &local, const ProposeMessage &remote,
      bool local_is_preferred) {
    Algorithm match{};

    using curveT = crypto::common::CurveType;
    const auto curve{
        BestMatch(local.exchanges, remote.exchanges, local_is_preferred)};
    if (!curve) {
      return Error::NO_COMMON_EC_ALGO;
    }
    if (*curve == "P-256") {
      match.curve = curveT::P256;
    } else if (*curve == "P-384") {
      match.curve = curveT::P384;
    } else if (*curve == "P-521") {
      match.curve = curveT::P521;
    } else {
      return Error::INTERNAL_FAILURE;
    }

    using cipherT = crypto::common::CipherType;
    const auto cipher{
        BestMatch(local.ciphers, remote.ciphers, local_is_preferred)};
    if (!cipher) {
      return Error::NO_COMMON_CIPHER_ALGO;
    }
    if (*cipher == "AES-256") {
      match.cipher = cipherT::AES256;
    } else if (*cipher == "AES-128") {
      match.cipher = cipherT::AES128;
    } else {
      return Error::INTERNAL_FAILURE;
    }

    using hashT = crypto::common::HashType;
    const auto hash{BestMatch(local.hashes, remote.hashes, local_is_preferred)};
    if (!hash) {
      return Error::NO_COMMON_HASH_ALGO;
    }
    if (*hash == "SHA256") {
      match.hash = hashT::SHA256;
    } else if (*hash == "SHA512") {
      match.hash = hashT::SHA512;
    } else {
      return Error::INTERNAL_FAILURE;
    }
    return match;
  }

  outcome::result<std::vector<uint8_t>> Dialer::getCorpus(
      bool for_local_peer,
      gsl::span<const uint8_t> ephemeral_public_key) const {
    if (!(local_peer_proposal_bytes_ && remote_peer_proposal_bytes_)) {
      return Error::INTERNAL_FAILURE;
    }

    size_t corpus_len{local_peer_proposal_bytes_->size()
                      + remote_peer_proposal_bytes_->size()
                      + ephemeral_public_key.size()};
    std::vector<uint8_t> corpus;
    corpus.reserve(corpus_len);
    if (for_local_peer) {
      corpus.insert(corpus.end(), local_peer_proposal_bytes_->begin(),
                    local_peer_proposal_bytes_->end());
      corpus.insert(corpus.end(), remote_peer_proposal_bytes_->begin(),
                    remote_peer_proposal_bytes_->end());
    } else {
      corpus.insert(corpus.end(), remote_peer_proposal_bytes_->begin(),
                    remote_peer_proposal_bytes_->end());
      corpus.insert(corpus.end(), local_peer_proposal_bytes_->begin(),
                    local_peer_proposal_bytes_->end());
    }
    corpus.insert(corpus.end(), ephemeral_public_key.begin(),
                  ephemeral_public_key.end());

    return corpus;
  }
}  // namespace libp2p::security::secio
